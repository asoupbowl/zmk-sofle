import AppKit
@preconcurrency import CoreBluetooth
import Foundation
import ServiceManagement

private let serviceUUID = CBUUID(string: "9C3DFA10-6A8B-4C72-BD2F-19E56F2A7001")
private let snapshotUUID = CBUUID(string: "9C3DFA11-6A8B-4C72-BD2F-19E56F2A7001")
private let hidServiceUUID = CBUUID(string: "1812")

final class BridgeLogger {
    static let shared = BridgeLogger()
    private let handle: FileHandle?

    private init() {
        let manager = FileManager.default
        let directory = manager.homeDirectoryForCurrentUser
            .appendingPathComponent("Library/Logs/Eyelash Sofle Codex Usage", isDirectory: true)
        try? manager.createDirectory(at: directory, withIntermediateDirectories: true)
        let file = directory.appendingPathComponent("wireless.log")
        if !manager.fileExists(atPath: file.path) {
            manager.createFile(atPath: file.path, contents: nil)
        }
        handle = try? FileHandle(forWritingTo: file)
        _ = try? handle?.seekToEnd()
    }

    func write(_ message: String) {
        let formatter = ISO8601DateFormatter()
        let line = "[\(formatter.string(from: Date()))] \(message)\n"
        if let data = line.data(using: .utf8) {
            try? handle?.write(contentsOf: data)
            try? handle?.synchronize()
        }
    }
}

final class WirelessBridge: NSObject, CBCentralManagerDelegate, CBPeripheralDelegate {
    private var central: CBCentralManager!
    private var keyboard: CBPeripheral?
    private var snapshotCharacteristic: CBCharacteristic?
    private var reconnectTimer: Timer?
    private var syncTimer: Timer?
    private var queryInFlight = false
    private let logger = BridgeLogger.shared

    override init() {
        super.init()
        central = CBCentralManager(delegate: self, queue: nil)
        reconnectTimer = Timer.scheduledTimer(withTimeInterval: 10, repeats: true) { [weak self] _ in
            self?.locateKeyboard()
        }
        syncTimer = Timer.scheduledTimer(withTimeInterval: 60, repeats: true) { [weak self] _ in
            self?.syncNow()
        }
    }

    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        logger.write("Bluetooth state=\(central.state.rawValue) authorization=\(CBCentralManager.authorization.rawValue)")
        if central.state == .poweredOn {
            locateKeyboard()
        }
    }

    private func locateKeyboard() {
        guard central.state == .poweredOn, snapshotCharacteristic == nil else { return }

        let connected = central.retrieveConnectedPeripherals(withServices: [serviceUUID])
        if let peripheral = connected.first {
            connect(peripheral)
            return
        }

        if !central.isScanning {
            logger.write("Scanning for the paired Sofle keyboard")
            central.scanForPeripherals(withServices: nil, options: [
                CBCentralManagerScanOptionAllowDuplicatesKey: false
            ])
        }
    }

    private func connect(_ peripheral: CBPeripheral) {
        central.stopScan()
        keyboard = peripheral
        keyboard?.delegate = self
        if peripheral.state == .connected {
            peripheral.discoverServices([serviceUUID])
            return
        }
        logger.write("Connecting to \(peripheral.name ?? peripheral.identifier.uuidString)")
        central.connect(peripheral)
    }

    func centralManager(
        _ central: CBCentralManager,
        didDiscover peripheral: CBPeripheral,
        advertisementData: [String: Any],
        rssi RSSI: NSNumber
    ) {
        let name = peripheral.name ?? advertisementData[CBAdvertisementDataLocalNameKey] as? String
        let advertisedServices = advertisementData[CBAdvertisementDataServiceUUIDsKey] as? [CBUUID] ?? []
        guard name?.localizedCaseInsensitiveContains("sofle") == true,
              advertisedServices.contains(hidServiceUUID) else { return }
        connect(peripheral)
    }

    func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        logger.write("Connected to \(peripheral.name ?? peripheral.identifier.uuidString)")
        peripheral.delegate = self
        peripheral.discoverServices([serviceUUID])
    }

    func centralManager(
        _ central: CBCentralManager,
        didFailToConnect peripheral: CBPeripheral,
        error: Error?
    ) {
        logger.write("Connection failed: \(error?.localizedDescription ?? "unknown error")")
        keyboard = nil
        snapshotCharacteristic = nil
    }

    func centralManager(
        _ central: CBCentralManager,
        didDisconnectPeripheral peripheral: CBPeripheral,
        error: Error?
    ) {
        logger.write("Disconnected: \(error?.localizedDescription ?? "normal")")
        keyboard = nil
        snapshotCharacteristic = nil
        locateKeyboard()
    }

    func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        if let error {
            logger.write("Service discovery failed: \(error.localizedDescription)")
            return
        }
        guard let service = peripheral.services?.first(where: { $0.uuid == serviceUUID }) else {
            logger.write("Codex BLE service was not found; the keyboard may need to be re-paired")
            return
        }
        peripheral.discoverCharacteristics([snapshotUUID], for: service)
    }

    func peripheral(
        _ peripheral: CBPeripheral,
        didDiscoverCharacteristicsFor service: CBService,
        error: Error?
    ) {
        if let error {
            logger.write("Characteristic discovery failed: \(error.localizedDescription)")
            return
        }
        guard let characteristic = service.characteristics?.first(where: { $0.uuid == snapshotUUID }) else {
            logger.write("Codex snapshot characteristic was not found")
            return
        }
        snapshotCharacteristic = characteristic
        logger.write("Wireless Codex channel is ready")
        syncNow()
    }

    private func syncNow() {
        guard !queryInFlight,
              let peripheral = keyboard,
              peripheral.state == .connected,
              let characteristic = snapshotCharacteristic else { return }

        queryInFlight = true
        DispatchQueue.global(qos: .utility).async { [weak self] in
            let result = self?.readSnapshot()
            DispatchQueue.main.async {
                guard let self else { return }
                self.queryInFlight = false
                switch result {
                case .success(let payload):
                    let type: CBCharacteristicWriteType = characteristic.properties.contains(.write)
                        ? .withResponse : .withoutResponse
                    peripheral.writeValue(payload, for: characteristic, type: type)
                case .failure(let error):
                    self.logger.write("Codex query failed: \(error.localizedDescription)")
                case .none:
                    break
                }
            }
        }
    }

    private func readSnapshot() -> Result<Data, Error> {
        guard let script = Bundle.main.url(
            forResource: "codex_usage_bridge",
            withExtension: "py",
            subdirectory: nil
        ) else {
            return .failure(NSError(domain: "EyelashSofleCodex", code: 1,
                                    userInfo: [NSLocalizedDescriptionKey: "Bundled bridge script is missing"]))
        }

        let process = Process()
        let output = Pipe()
        process.executableURL = URL(fileURLWithPath: "/usr/bin/python3")
        process.arguments = [script.path, "--once", "--dry-run"]
        process.standardOutput = output
        process.standardError = output

        do {
            try process.run()
            process.waitUntilExit()
            let data = output.fileHandleForReading.readDataToEndOfFile()
            let text = String(decoding: data, as: UTF8.self)
            guard process.terminationStatus == 0,
                  let frame = text.split(whereSeparator: \.isNewline).first(where: { $0.hasPrefix("CX1,") })
            else {
                throw NSError(domain: "EyelashSofleCodex", code: 2,
                              userInfo: [NSLocalizedDescriptionKey: text.trimmingCharacters(in: .whitespacesAndNewlines)])
            }

            let payloadText = frame.split(separator: "*", maxSplits: 1)[0]
            let fields = payloadText.split(separator: ",")
            guard fields.count == 3,
                  let param1 = UInt32(fields[1], radix: 16),
                  let param2 = UInt32(fields[2], radix: 16) else {
                throw NSError(domain: "EyelashSofleCodex", code: 3,
                              userInfo: [NSLocalizedDescriptionKey: "Invalid Codex frame"])
            }

            var first = param1.littleEndian
            var second = param2.littleEndian
            var snapshot = withUnsafeBytes(of: &first) { Data($0) }
            snapshot.append(withUnsafeBytes(of: &second) { Data($0) })
            return .success(snapshot)
        } catch {
            return .failure(error)
        }
    }

    func peripheral(
        _ peripheral: CBPeripheral,
        didWriteValueFor characteristic: CBCharacteristic,
        error: Error?
    ) {
        if let error {
            logger.write("Wireless write failed: \(error.localizedDescription)")
        } else {
            logger.write("Codex usage sent wirelessly")
        }
    }
}

final class AppDelegate: NSObject, NSApplicationDelegate {
    private var bridge: WirelessBridge?

    func applicationDidFinishLaunching(_ notification: Notification) {
        if #available(macOS 13.0, *) {
            do {
                if SMAppService.mainApp.status != .enabled {
                    try SMAppService.mainApp.register()
                }
                BridgeLogger.shared.write("Login item status=\(SMAppService.mainApp.status.rawValue)")
            } catch {
                BridgeLogger.shared.write("Login item registration failed: \(error.localizedDescription)")
            }
        }
        bridge = WirelessBridge()
    }
}

let application = NSApplication.shared
let delegate = AppDelegate()
application.delegate = delegate
application.setActivationPolicy(.accessory)
application.run()
