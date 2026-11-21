cmake_minimum_required(VERSION 3.24)
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

# SDL 3: use an installed SDL of at least this version, otherwise build the pinned release from source
# (statically linked; on Linux it loads X11, Wayland, PulseAudio and so on at runtime when present)
set(DERCLOU_SDL_VERSION 3.4.20)
option(DERCLOU_BUNDLED_SDL "Always build SDL ${DERCLOU_SDL_VERSION} from source" OFF)
if(NOT DERCLOU_BUNDLED_SDL AND NOT EMSCRIPTEN)
  find_package(SDL3 ${DERCLOU_SDL_VERSION} CONFIG QUIET)
endif()
if(SDL3_FOUND)
  message(STATUS "Using installed SDL ${SDL3_VERSION}")
else()
  message(STATUS "Building SDL ${DERCLOU_SDL_VERSION} from source")
  include(FetchContent)
  FetchContent_Declare(
    SDL3
    URL https://github.com/libsdl-org/SDL/releases/download/release-${DERCLOU_SDL_VERSION}/SDL3-${DERCLOU_SDL_VERSION}.tar.gz
    URL_HASH SHA256=f3bb06f55bed9557227a700af2ea62e49497ebdf58037732a731a5fb3ef8581f
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
  set(SDL_SHARED OFF CACHE BOOL "" FORCE)
  set(SDL_STATIC ON CACHE BOOL "" FORCE)
  set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
  set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(SDL_TESTS OFF CACHE BOOL "" FORCE)
  set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(SDL3)
endif()

# Lua for the scripts of Game+: the pinned release is always built in (it is small and has no dependencies)
set(DERCLOU_LUA_VERSION 5.5.1)
include(FetchContent)
FetchContent_Declare(
  lua
  URL https://www.lua.org/ftp/lua-${DERCLOU_LUA_VERSION}.tar.gz
  URL_HASH SHA256=1c4b4068d67061f2a2231ad2b5422e77acea1487ea9890f6320af614f4373dce
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(lua) # the tarball has no CMakeLists.txt, this only downloads it
file(GLOB LUA_SOURCES ${lua_SOURCE_DIR}/src/*.c)
list(REMOVE_ITEM LUA_SOURCES ${lua_SOURCE_DIR}/src/lua.c ${lua_SOURCE_DIR}/src/luac.c)
add_library(lua_static STATIC ${LUA_SOURCES})
target_include_directories(lua_static SYSTEM PUBLIC ${lua_SOURCE_DIR}/src)
if(APPLE)
  target_compile_definitions(lua_static PRIVATE LUA_USE_MACOSX)
elseif(UNIX AND NOT EMSCRIPTEN)
  target_compile_definitions(lua_static PRIVATE LUA_USE_POSIX)
endif()
if(UNIX AND NOT EMSCRIPTEN)
  target_link_libraries(lua_static PUBLIC m)
endif()
add_library(Lua::Lua ALIAS lua_static)
message(STATUS "Lua ${DERCLOU_LUA_VERSION}")

if(EMSCRIPTEN)
  # Release: optimise, no debug info (keeps the download small)
  set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG" CACHE STRING "Release flags" FORCE)

  set(LINK_FLAGS "-sASYNCIFY -sALLOW_MEMORY_GROWTH=1 \
    -sINVOKE_RUN=0 -sEXPORTED_RUNTIME_METHODS=callMain,FS,ENV \
    --preload-file ${CMAKE_CURRENT_SOURCE_DIR}/gamedata@/gamedata \
    --shell-file ${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html \
    -lidbfs.js")

  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LINK_FLAGS}")

  set(CMAKE_EXECUTABLE_SUFFIX .html)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/web)
else()
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/x64)
endif()

subdirs(src)

if(NOT DEFINED INSTALL_PATH_DATA)
  set(INSTALL_PATH_DATA share/DerClou)
endif()
message(STATUS "Install path data is ${INSTALL_PATH_DATA}")
install(DIRECTORY gamedata/ DESTINATION ${INSTALL_PATH_DATA})
