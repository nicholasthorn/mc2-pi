cmake_minimum_required (VERSION 3.10)
project(gameos)

set(SOURCES ${SOURCES}
    gameos.cpp
    gameos_graphics.cpp
    gameos_res.cpp
    gameos_fileio.cpp
    gameos_input.cpp
    gameos_debugging.cpp
    gameos_sound.cpp
    gos_render.cpp
    gos_font.cpp
    gos_input.cpp

    utils/stream.cpp
    utils/camera.cpp
    utils/gl_utils.cpp
    utils/pixel_repack.cpp
    utils/audio_pan.cpp
    utils/jpeg_decode.cpp
    utils/stb_image_impl.cpp
    utils/pe_strings.cpp
    utils/str_bounded.cpp
    utils/stereo.cpp
    utils/args_file.cpp
    utils/gl_render_constants.cpp
    utils/Image.cpp
    utils/logging.cpp
    utils/shader_builder.cpp
    utils/matrix.cpp
    utils/vec.cpp
    utils/timing.cpp
    utils/string_utils.cpp
    utils/file_utils.cpp
    )

# frame_stats and tga_writer are only used by the main loop (-bench). The link
# line lists gameos before gameos_main, so in gameos they would never be
# pulled in.
set(MAIN_SRC gameosmain.cpp utils/frame_stats.cpp utils/tga_writer.cpp)

#find_package(SDL2 REQUIRED PATHS "/opt" NO_DEFAULT_PATH)
#include_directories("${SDL2_PREFIX}/include")

include_directories("../include"  ".")
# thirdparty/stb (this directory is added before the top level adds thirdparty)
include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/thirdparty")
include_directories(${THIRDPARTY_INCLUDE_DIRS})

add_library(gameos ${SOURCES})
add_library(gameos_main ${MAIN_SRC})

# new, GL-free code: strict warnings (see MC2_STRICT_WARNINGS)
set_source_files_properties(utils/pixel_repack.cpp utils/audio_pan.cpp utils/frame_stats.cpp utils/tga_writer.cpp
    utils/jpeg_decode.cpp utils/pe_strings.cpp utils/str_bounded.cpp utils/stereo.cpp utils/args_file.cpp
    PROPERTIES COMPILE_OPTIONS "${MC2_STRICT_WARNINGS}")

