# SPDX-License-Identifier: MIT

if(CONFIG_ZMK_CODEX_USAGE)
  target_include_directories(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)
  target_sources(app PRIVATE
    src/codex_usage_state.c
    src/events_codex_usage_changed.c
  )
  target_sources_ifdef(CONFIG_ZMK_CODEX_USAGE_HOST_UART app PRIVATE src/host_uart.c)
  target_sources_ifdef(CONFIG_ZMK_CODEX_USAGE_STATUS_SCREEN app PRIVATE
    src/codex_usage_widget.c
    src/status_screen.c
  )
endif()
