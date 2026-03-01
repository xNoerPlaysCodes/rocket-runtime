# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles/RGEditor_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/RGEditor_autogen.dir/ParseCache.txt"
  "RGEditor_autogen"
  )
endif()
