# FIXME: Make other platforms work
STRING (REGEX MATCH "Linux" IS_LINUX ${CMAKE_SYSTEM_NAME})

if(IS_LINUX)
  add_subdirectory(linux)
elseif(WIN32)
  add_subdirectory(windows)
else()
  message(FATAL_ERROR "HID backend not supported for this platform")
endif()
