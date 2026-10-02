# Regression check: production executable must embed resources.qrc content.
#
# Guards against AUTORCC being disabled (or .qrc silently dropped), which
# historically left the shipped binary without QSS/icons while UnitTests
# still passed via explicit qt_add_resources.
#
# Invoked as:
#   cmake -DAPP_EXE=<path-to-production-exe> -P cmake/VerifyProductionResources.cmake

if(NOT DEFINED APP_EXE OR APP_EXE STREQUAL "")
  message(FATAL_ERROR "APP_EXE is not set")
endif()

if(NOT EXISTS "${APP_EXE}")
  message(FATAL_ERROR "Production executable not found: ${APP_EXE}")
endif()

file(READ "${APP_EXE}" exe_hex HEX)

# Qt rcc stores resource names as UTF-16BE in the name table.
# dark.qss  -> 0064 0061 0072 006b 002e 0071 0073 0073
# app_icon.png -> 0061 0070 0070 005f 0069 0063 006f 006e 002e 0070 006e 0067
set(_dark_qss_utf16be "006400610072006b002e007100730073")
set(_icon_png_utf16be
    "006100700070005f00690063006f006e002e0070006e0067")

string(FIND "${exe_hex}" "${_dark_qss_utf16be}" _dark_pos)
string(FIND "${exe_hex}" "${_icon_png_utf16be}" _icon_pos)

if(_dark_pos EQUAL -1)
  message(FATAL_ERROR
    "Production executable is missing embedded :/styles/windows/dark.qss "
    "(rcc name table). AUTORCC/resources.qrc regression? Exe: ${APP_EXE}")
endif()

if(_icon_pos EQUAL -1)
  message(FATAL_ERROR
    "Production executable is missing embedded :/icons/app_icon.png "
    "(rcc name table). AUTORCC/resources.qrc regression? Exe: ${APP_EXE}")
endif()

message(STATUS
  "Production resources OK: dark.qss @ ${_dark_pos}, app_icon.png @ ${_icon_pos} (${APP_EXE})")
