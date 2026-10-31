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
    test_paths.cpp
    test_pixel_repack.cpp
    test_routines.cpp
    test_audio_pan.cpp
    test_globalmapio.cpp
    test_frame_stats.cpp
    test_render_heuristics.cpp

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
    ${MC2_SRC}/mclib/routines.cpp      # RollDice / RandomNumber (gos_rand from test_routines.cpp)

    # GL- and SDL-free GameOS helpers
    ${MC2_SRC}/GameOS/gameos/utils/pixel_repack.cpp
    ${MC2_SRC}/GameOS/gameos/utils/audio_pan.cpp
    ${MC2_SRC}/GameOS/gameos/utils/frame_stats.cpp
)

# Engine headers are legacy code: include them as SYSTEM headers (replacing the
# plain -I paths inherited from the top-level include_directories) so the
# strict warnings below report the test code, not the 2001 headers it uses.
get_target_property(_mc2_engine_includes mc2_tests INCLUDE_DIRECTORIES)
set_target_properties(mc2_tests PROPERTIES INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(mc2_tests SYSTEM PRIVATE
    ${_mc2_engine_includes}
    ${MC2_SRC}/mclib
    ${MC2_SRC}/GameOS/include
    ${MC2_SRC}/GameOS/gameos
)

# New code in this target is held to -Wall -Wextra -Werror (GCC/Clang); the
# legacy engine .cpp files above keep the global flags.
set_source_files_properties(
    test_main.cpp heap_shim.cpp test_file_inifile.cpp test_csvfile.cpp
    test_fst_roundtrip.cpp test_paths.cpp test_pixel_repack.cpp test_routines.cpp
    test_audio_pan.cpp test_globalmapio.cpp test_frame_stats.cpp test_render_heuristics.cpp
    ${MC2_SRC}/GameOS/gameos/utils/pixel_repack.cpp
    ${MC2_SRC}/GameOS/gameos/utils/audio_pan.cpp
    ${MC2_SRC}/GameOS/gameos/utils/frame_stats.cpp
    PROPERTIES COMPILE_OPTIONS "${MC2_STRICT_WARNINGS}")

# Fixtures are copied next to the tests and opened by RELATIVE path:
# File::open lowercases the whole name, so an absolute path with upper-case
# directories (e.g. /mnt/c/Users/...) would not resolve on Linux.
file(COPY fixtures DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
target_compile_definitions(mc2_tests PRIVATE MC2_TEST_FIXTURE_DIR="fixtures")

# windows = GameOS/src Win32 emulation (S_strlwr, _filelength, MessageBoxA...)
target_link_libraries(mc2_tests PRIVATE windows ZLIB::ZLIB ${ADDITIONAL_LIBS})

add_test(NAME mc2_tests COMMAND mc2_tests WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
