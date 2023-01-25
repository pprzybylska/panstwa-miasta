# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles/panstwa-miasta_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/panstwa-miasta_autogen.dir/ParseCache.txt"
  "panstwa-miasta_autogen"
  )
endif()
