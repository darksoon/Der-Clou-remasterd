set(SOURCE_FILES
    sound/mxr.c
    sound/mxr_voc.c
    sound/mxr_wav.c
    sound/mxr_hsc.c
    sound/fx.c
    sound/newsound.c
    sound/fmopl.c
    sound/opl.c
    sound/nuked/opl3.c
    anim/sysanim.c
    base/base.c
    base/settings.c
    script/script.c
    data/dataappl.c
    data/database.c
    data/datacalc.c
    data/relation.c
    dialog/dialog.c
    dialog/talkappl.c
    disk/disk.c
    error/error.c
    gameplay/gp.c
    gameplay/gp_app.c
    gameplay/loadsave.c
    gameplay/tcreques.c
    gfx/gfx.c
    gfx/gfxmcga.c
    gfx/gfxnch4.c
    inphdl/inphdl.c
    landscap/access.c
    landscap/hardware.c
    landscap/init.c
    landscap/landscap.c
    landscap/raster.c
    landscap/scroll.c
    landscap/spot.c
    list/list.c
    living/bob.c
    living/living.c
    memory/memory.c
    organisa/organisa.c
    planing/graphics.c
    planing/guards.c
    planing/io.c
    planing/main.c
    planing/planer.c
    planing/player.c
    planing/plus.c
    planing/prepare.c
    planing/support.c
    planing/sync.c
    planing/system.c
    present/interac.c
    present/present.c
    random/random.c
    scenes/cars.c
    scenes/dealer.c
    scenes/done.c
    scenes/evidence.c
    scenes/inside.c
    scenes/invest.c
    scenes/scenes.c
    scenes/tools.c
    story/story.c
    text/text.c
    port/port.c
    intro/intro.c
    intro/iffanim.c
    gfx/filter.c
    gfx/crt.c
    gfx/loadimage.c
    theclou.c)

add_executable(DerClou ${SOURCE_FILES})
if(EMSCRIPTEN)
  # relink when the page template changes
  set_target_properties(DerClou PROPERTIES LINK_DEPENDS "${CMAKE_SOURCE_DIR}/web/shell.html")
  # the Game+ voices are a package of their own: together with the game data one file would pass the
  # 100 MB limit of GitHub Pages. The page loads voices.js before the game starts.
  add_custom_command(TARGET DerClou POST_BUILD
    COMMAND ${EMSCRIPTEN_ROOT_PATH}/tools/file_packager voices.data
            --preload ${CMAKE_SOURCE_DIR}/gamedata/PLUS/VOICE@/gamedata/PLUS/VOICE
            --js-output=voices.js
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    COMMENT "Packing the Game+ voices")
endif()

target_include_directories(DerClou PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_compile_options(DerClou PUBLIC -Wall -Wno-format-overflow)

add_compile_options("$<$<CONFIG:DEBUG>:-g3 -ggdb>")
add_compile_options("$<$<CONFIG:RELEASE>:-O2>")
add_link_options("$<$<CONFIG:RELEASE>:-s>")
add_compile_definitions(CMAKE_INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}"
                        INSTALL_PATH_BIN="${INSTALL_PATH_BIN}")

target_link_libraries(DerClou PRIVATE SDL3::SDL3 Lua::Lua)
if(UNIX AND NOT APPLE AND NOT EMSCRIPTEN)
  target_link_libraries(DerClou PRIVATE m)
endif()

if(NOT DEFINED INSTALL_PATH_BIN)
  set(INSTALL_PATH_BIN ${CMAKE_INSTALL_BINDIR})
endif()

install(TARGETS DerClou DESTINATION ${INSTALL_PATH_BIN})
