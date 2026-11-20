cmake_minimum_required(VERSION 3.12)
include(CMakePrintHelpers)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" DERCLOU_VERSION LIMIT_COUNT 1)

project(
  DerClou
  VERSION ${DERCLOU_VERSION}
  LANGUAGES C)

# version string shown in the game: "0.9.0" or "0.9.0-12-gabc1234" between releases
set(DERCLOU_VERSION_FULL "${DERCLOU_VERSION}")
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
  execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --tags --match "v*" --dirty=-mod
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE DERCLOU_GIT_DESCRIBE
    OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
  if(DERCLOU_GIT_DESCRIBE MATCHES "^v(.+)$")
    set(DERCLOU_VERSION_FULL "${CMAKE_MATCH_1}")
  endif()
endif()
message(STATUS "Der Clou! remastered version ${DERCLOU_VERSION_FULL}")
add_compile_definitions(DERCLOU_VERSION="${DERCLOU_VERSION_FULL}")

cmake_print_variables(CMAKE_C_COMPILER)
cmake_print_variables(CMAKE_SYSTEM_NAME)
if( ${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
  set(CMAKE_C_COMPILER "emcc")

  # Override default optimization for release builds
  set(CMAKE_C_FLAGS_RELEASE "-O1 -DNDEBUG" CACHE STRING "Release flags" FORCE)
  
  set(COMPILE_FLAGS "-g --use-port=sdl2 -sASYNCIFY -sLEGACY_GL_EMULATION --use-port=sdl2_image:formats=lbm,png")
  set(LINK_FLAGS "-sASYNCIFY -sLEGACY_GL_EMULATION --preload-file ../../gamedata -lidbfs.js")

  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMPILE_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LINK_FLAGS}")

  set(CMAKE_EXECUTABLE_SUFFIX .html)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/web)
else()
  find_package(SDL2 REQUIRED CONFIG)
  find_package(SDL2_image REQUIRED)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/x64)
endif()

subdirs(src)

if(NOT DEFINED INSTALL_PATH_DATA)
  set(INSTALL_PATH_DATA share/DerClou)
endif()
message(STATUS "Install path data is ${INSTALL_PATH_DATA}")
install(DIRECTORY gamedata/ DESTINATION ${INSTALL_PATH_DATA})
