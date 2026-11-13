# tests/unit: doctest unit tests for GL-free engine code, run through CTest.
#
# Built as part of the main build (MC2_BUILD_TESTS, default ON) so the tests
# see exactly the production compile environment: LINUX_BUILD,
# USE_ASSEMBLER_CODE=0, -fsigned-char, PLATFORM_WINDOWS on MSVC, MC2_GLES.
#
#   cmake --build build && ctest --test-dir build --output-on-failure
#
# Harness ported from mc2-opengl-remastered tests/unit. heap_shim.cpp stands
# in for mclib/heap.cpp and the GameOS globals, so real engine TUs link
# without the game, SDL or GL. Never add heap.cpp to this target (ODR).

set(MC2_SRC "${CMAKE_SOURCE_DIR}")

add_executable(mc2_tests
    test_main.cpp            # owns DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
    heap_shim.cpp
    test_file_inifile.cpp
    test_csvfile.cpp
    test_fst_roundtrip.cpp
    test_lzdecomp.cpp
    test_paths.cpp
    test_pixel_repack.cpp
    test_routines.cpp
    test_audio_pan.cpp
    test_globalmapio.cpp
    test_jpeg_decode.cpp
    test_pe_strings.cpp
    test_typename.cpp
    test_missaim.cpp
    test_mechtypefit.cpp
    test_iconsheet.cpp
    test_str_bounded.cpp
    test_frame_stats.cpp
    test_benchscenario.cpp
    test_tga_writer.cpp
    test_render_heuristics.cpp
    test_hud_layout.cpp
    test_uismokewalk.cpp
    test_image_tga.cpp
    test_stereo.cpp
    test_vr_eye.cpp
    test_results_layout.cpp
    test_vr_hud.cpp
    test_vr_edge_fade.cpp
    test_vr_controls.cpp
    test_input_latch.cpp
    test_args_file.cpp
    test_log_file.cpp
    test_csvfield.cpp
    test_estring.cpp
    test_wave_parse.cpp
    test_pqueue.cpp
    test_netwire.cpp
    test_netlan.cpp
    test_netlobby.cpp
    test_gos_net.cpp
    test_netrules.cpp
    test_netforce.cpp
    test_netsession.cpp
    test_gos_net_gns.cpp
    test_tga_readers.cpp
    test_move_save_bounds.cpp
    test_forcegroupcycle.cpp
    test_txmcachename.cpp
    test_fuzz_replay.cpp
    test_ablscan.cpp

    # Fuzz targets (fuzz/fuzz_targets.h), replayed by test_fuzz_replay.cpp
    # and built as libFuzzer executables with MC2_FUZZ.
    fuzz/fuzz_support.cpp
    fuzz/fuzz_fit.cpp
    fuzz/fuzz_csv.cpp
    fuzz/fuzz_tga.cpp
    fuzz/fuzz_abl.cpp

    # Real engine TUs: the file / ini / packet / FST / CSV stack. This is the
    # whole link frontier; nothing else from mclib or GameOS is needed.
    ${MC2_SRC}/mclib/file.cpp
    ${MC2_SRC}/mclib/inifile.cpp
    ${MC2_SRC}/mclib/packet.cpp
    ${MC2_SRC}/mclib/ffile.cpp
    ${MC2_SRC}/mclib/fastfile.cpp
    ${MC2_SRC}/mclib/lzdecomp.cpp
    ${MC2_SRC}/mclib/lzcomp.cpp
    ${MC2_SRC}/mclib/err.cpp
    ${MC2_SRC}/mclib/csvfile.cpp
    ${MC2_SRC}/mclib/paths.cpp
    ${MC2_SRC}/mclib/estring.cpp
    ${MC2_SRC}/mclib/routines.cpp      # RollDice / RandomNumber (gos_rand from test_routines.cpp)
    ${MC2_SRC}/mclib/waveparse.cpp     # wave_ParseWaveMemory (sound bite chunk walk)
    ${MC2_SRC}/mclib/pqueue.cpp        # PriorityQueue (A* open list)
    ${MC2_SRC}/mclib/tgainfo.cpp       # terrain tile and mask TGA readers
    # the ABL compiler and VM, for fuzz/fuzz_abl.cpp
    ${MC2_SRC}/mclib/abldbug.cpp
    ${MC2_SRC}/mclib/abldecl.cpp
    ${MC2_SRC}/mclib/ablenv.cpp
    ${MC2_SRC}/mclib/ablerr.cpp
    ${MC2_SRC}/mclib/ablexec.cpp
    ${MC2_SRC}/mclib/ablexpr.cpp
    ${MC2_SRC}/mclib/ablrtn.cpp
    ${MC2_SRC}/mclib/ablscan.cpp
    ${MC2_SRC}/mclib/ablstd.cpp
    ${MC2_SRC}/mclib/ablstmt.cpp
    ${MC2_SRC}/mclib/ablsymt.cpp
    ${MC2_SRC}/mclib/ablxexpr.cpp
    ${MC2_SRC}/mclib/ablxstd.cpp
    ${MC2_SRC}/mclib/ablxstmt.cpp

    # GL- and SDL-free GameOS helpers
    ${MC2_SRC}/GameOS/gameos/utils/pixel_repack.cpp
    ${MC2_SRC}/GameOS/gameos/utils/audio_pan.cpp
    ${MC2_SRC}/GameOS/gameos/utils/jpeg_decode.cpp
    ${MC2_SRC}/GameOS/gameos/utils/stb_image_impl.cpp
    ${MC2_SRC}/GameOS/gameos/utils/pe_strings.cpp
    ${MC2_SRC}/GameOS/gameos/utils/str_bounded.cpp
    ${MC2_SRC}/GameOS/gameos/utils/Image.cpp
    ${MC2_SRC}/GameOS/gameos/utils/frame_stats.cpp
    ${MC2_SRC}/GameOS/gameos/utils/tga_writer.cpp
    ${MC2_SRC}/GameOS/gameos/utils/stereo.cpp
    ${MC2_SRC}/GameOS/gameos/utils/vr_eye.cpp
    ${MC2_SRC}/GameOS/gameos/utils/vr_controls.cpp
    ${MC2_SRC}/GameOS/gameos/utils/args_file.cpp
    ${MC2_SRC}/GameOS/gameos/utils/log_file.cpp
    ${MC2_SRC}/GameOS/gameos/gos_net.cpp
    ${MC2_SRC}/GameOS/gameos/gos_net_lan.cpp

    # scripted benchmark scenario and UI smoke walk: game code, but game-free
    ${MC2_SRC}/code/benchscenario.cpp
    ${MC2_SRC}/code/uismokewalk.cpp
    ${MC2_SRC}/code/netwire.cpp
    ${MC2_SRC}/code/netsession.cpp
    ${MC2_SRC}/code/netlan.cpp
    ${MC2_SRC}/code/netlobby.cpp
    ${MC2_SRC}/code/netrules.cpp
    ${MC2_SRC}/code/netforce.cpp
    ${MC2_SRC}/code/iconsheet.cpp
    ${MC2_SRC}/code/typename.cpp
    ${MC2_SRC}/code/missaim.cpp
    ${MC2_SRC}/code/mechtypefit.cpp
    ${MC2_SRC}/code/forcegroupcycle.cpp
)

# Engine headers are legacy code: include them as SYSTEM headers (replacing the
# plain -I paths inherited from the top-level include_directories) so the
# strict warnings below report the test code, not the 2001 headers it uses.
get_target_property(_mc2_engine_includes mc2_tests INCLUDE_DIRECTORIES)
function(mc2_test_includes target)
    set_target_properties(${target} PROPERTIES INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}")
    target_include_directories(${target} SYSTEM PRIVATE
        ${_mc2_engine_includes}
        ${MC2_SRC}/mclib
        ${MC2_SRC}/GameOS/include
        ${MC2_SRC}/GameOS/gameos
        ${MC2_SRC}/thirdparty
        ${MC2_SRC}/code
    )
endfunction()
mc2_test_includes(mc2_tests)

# New code in this target is held to -Wall -Wextra -Werror (GCC/Clang); the
# legacy engine .cpp files above keep the global flags.
set_source_files_properties(
    test_main.cpp heap_shim.cpp test_file_inifile.cpp test_csvfile.cpp
    test_fst_roundtrip.cpp test_lzdecomp.cpp test_paths.cpp test_pixel_repack.cpp test_routines.cpp
    test_audio_pan.cpp test_globalmapio.cpp test_frame_stats.cpp test_benchscenario.cpp
    test_uismokewalk.cpp test_iconsheet.cpp test_vr_edge_fade.cpp
    test_tga_writer.cpp test_render_heuristics.cpp test_hud_layout.cpp test_jpeg_decode.cpp test_pe_strings.cpp test_typename.cpp test_missaim.cpp test_mechtypefit.cpp test_str_bounded.cpp test_image_tga.cpp test_stereo.cpp test_vr_eye.cpp test_vr_controls.cpp test_input_latch.cpp test_csvfield.cpp test_args_file.cpp test_log_file.cpp test_estring.cpp test_wave_parse.cpp test_pqueue.cpp test_tga_readers.cpp test_move_save_bounds.cpp test_txmcachename.cpp test_forcegroupcycle.cpp
    test_fuzz_replay.cpp test_ablscan.cpp fuzz/fuzz_support.cpp fuzz/fuzz_fit.cpp fuzz/fuzz_csv.cpp fuzz/fuzz_tga.cpp fuzz/fuzz_abl.cpp
    fuzz/libfuzzer_main.cpp fuzz/fuzz_gos_stubs.cpp
    test_netwire.cpp test_netlan.cpp test_netlobby.cpp test_netsession.cpp test_netrules.cpp test_netforce.cpp test_gos_net.cpp test_gos_net_gns.cpp
    ${MC2_SRC}/GameOS/gameos/utils/pixel_repack.cpp
    ${MC2_SRC}/GameOS/gameos/utils/audio_pan.cpp
    ${MC2_SRC}/GameOS/gameos/utils/frame_stats.cpp
    ${MC2_SRC}/GameOS/gameos/utils/tga_writer.cpp
    ${MC2_SRC}/GameOS/gameos/utils/stereo.cpp
    ${MC2_SRC}/GameOS/gameos/utils/vr_eye.cpp
    ${MC2_SRC}/GameOS/gameos/utils/vr_controls.cpp
    ${MC2_SRC}/GameOS/gameos/utils/args_file.cpp
    ${MC2_SRC}/GameOS/gameos/utils/log_file.cpp
    ${MC2_SRC}/code/benchscenario.cpp
    ${MC2_SRC}/code/uismokewalk.cpp
    ${MC2_SRC}/code/netwire.cpp
    ${MC2_SRC}/code/netsession.cpp
    ${MC2_SRC}/code/netlan.cpp
    ${MC2_SRC}/code/netlobby.cpp
    ${MC2_SRC}/code/netrules.cpp
    ${MC2_SRC}/code/netforce.cpp
    ${MC2_SRC}/GameOS/gameos/gos_net.cpp
    ${MC2_SRC}/GameOS/gameos/gos_net_lan.cpp
    ${MC2_SRC}/code/iconsheet.cpp
    ${MC2_SRC}/code/typename.cpp
    ${MC2_SRC}/code/missaim.cpp
    ${MC2_SRC}/code/mechtypefit.cpp
    ${MC2_SRC}/code/forcegroupcycle.cpp
    ${MC2_SRC}/GameOS/gameos/utils/jpeg_decode.cpp
    ${MC2_SRC}/GameOS/gameos/utils/pe_strings.cpp
    ${MC2_SRC}/GameOS/gameos/utils/str_bounded.cpp
    PROPERTIES COMPILE_OPTIONS "${MC2_STRICT_WARNINGS}")

# Fixtures are copied next to the tests and opened by RELATIVE path:
# File::open lowercases the whole name, so an absolute path with upper-case
# directories (e.g. /mnt/c/Users/...) would not resolve on Linux.
# Copied afresh on every build (not at configure time), so an edited,
# renamed or deleted fixture is never tested from a stale copy.
add_custom_target(mc2_test_fixtures ALL
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/fixtures
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/fixtures ${CMAKE_CURRENT_BINARY_DIR}/fixtures
    # the shipped bench scenarios, so a test can check they still load
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${MC2_SRC}/bench ${CMAKE_CURRENT_BINARY_DIR}/fixtures/bench
    # the fuzz seeds, replayed by test_fuzz_replay.cpp
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/fuzz/corpus ${CMAKE_CURRENT_BINARY_DIR}/fixtures/fuzz_corpus)

# The seed list for test_fuzz_replay.cpp ("<target> <path under fixtures/>"),
# which builds as C++0x and so cannot list a directory itself. Re-globbed on
# every build, so a new seed is picked up without reconfiguring by hand.
file(GLOB _mc2_fuzz_seeds CONFIGURE_DEPENDS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/fuzz/corpus
     ${CMAKE_CURRENT_SOURCE_DIR}/fuzz/corpus/*/*)
set(_mc2_fuzz_list "")
foreach(_seed ${_mc2_fuzz_seeds})
    string(REGEX REPLACE "/.*" "" _target "${_seed}")
    string(APPEND _mc2_fuzz_list "${_target} fuzz_corpus/${_seed}\n")
endforeach()
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/fuzz_corpus.txt "${_mc2_fuzz_list}")
add_dependencies(mc2_tests mc2_test_fixtures)
target_compile_definitions(mc2_tests PRIVATE MC2_TEST_FIXTURE_DIR="fixtures")

# windows = GameOS/src Win32 emulation (S_strlwr, _filelength, MessageBoxA...)
target_link_libraries(mc2_tests PRIVATE windows ZLIB::ZLIB ${ADDITIONAL_LIBS})
if(MSVC)
    target_link_libraries(mc2_tests PRIVATE ws2_32)	# gos_net_lan.cpp
endif()

add_test(NAME mc2_tests COMMAND mc2_tests WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

if(MC2_NET)
    target_sources(mc2_tests PRIVATE ${MC2_SRC}/GameOS/gameos/gos_net_gns.cpp)
    set_source_files_properties(${MC2_SRC}/GameOS/gameos/gos_net_gns.cpp
        PROPERTIES COMPILE_OPTIONS "${MC2_GNS_CALLER_OPTIONS}")
    target_link_libraries(mc2_tests PRIVATE mc2_gns)
    # built with the tests, since CI asks for mc2 and mc2_tests by name
    add_dependencies(mc2_tests net_ping)
    add_test(NAME net_two_process
        COMMAND ${CMAKE_COMMAND} -DNET_PING=$<TARGET_FILE:net_ping> -P ${MC2_SRC}/tests/net/two_process.cmake)
    set_tests_properties(net_two_process PROPERTIES TIMEOUT 90)
endif()

# libFuzzer executables (-DMC2_FUZZ=ON, Clang; fuzz/README.md): the same
# engine files and shim as mc2_tests, without the tests, one per target.
if(MC2_FUZZ)
    get_target_property(_mc2_test_sources mc2_tests SOURCES)
    set(_mc2_fuzz_sources "")
    foreach(_src ${_mc2_test_sources})
        get_filename_component(_name "${_src}" NAME)
        if(NOT _name MATCHES "^test_" AND NOT _name STREQUAL "gos_net_gns.cpp")
            list(APPEND _mc2_fuzz_sources "${_src}")
        endif()
    endforeach()
    # <target>:<function in fuzz_targets.h>
    foreach(_pair fit:fuzzFitFile csv:fuzzCsvFile tga:fuzzTga abl:fuzzAblScript)
        string(REPLACE ":" ";" _pair "${_pair}")
        list(GET _pair 0 _target)
        list(GET _pair 1 _fn)
        add_executable(mc2_fuzz_${_target} ${_mc2_fuzz_sources} fuzz/libfuzzer_main.cpp fuzz/fuzz_gos_stubs.cpp)
        mc2_test_includes(mc2_fuzz_${_target})
        target_compile_definitions(mc2_fuzz_${_target} PRIVATE MC2_FUZZ_TARGET=${_fn})
        target_link_options(mc2_fuzz_${_target} PRIVATE -fsanitize=fuzzer)
        target_link_libraries(mc2_fuzz_${_target} PRIVATE windows ZLIB::ZLIB ${ADDITIONAL_LIBS})
    endforeach()
endif()
