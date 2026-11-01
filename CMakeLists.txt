cmake_minimum_required (VERSION 3.10)
# select GLVND versus legacy OpenGL libraries
cmake_policy(SET CMP0072 NEW) # cmake --help-policy CMP0072
project(mc2)

include(CMakeToolsHelpers OPTIONAL)

# Set a default build type for single-configuration
# CMake generators if no build type is set.
#IF(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
#   SET(CMAKE_BUILD_TYPE RelWithDebInfo)
#ENDIF(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)

message("CMAKE library architecture: ${CMAKE_LIBRARY_ARCHITECTURE}")

if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
	message(STATUS "Target is 64 bits")
	set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS TRUE)
else("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
	message(STATUS "Target is 32 bits")
endif("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")

set(CMAKE_EXPORT_COMPILE_COMMANDS "ON")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_PREFIX_PATH}/cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/sdl2)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
# may have to help find_package bydetting path to FindSDL2.cmake
#set(SDL2_DIR "${CMAKE_PREFIX_PATH}/cmake")

if(NOT MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -ggdb")
    # (-fpermissive was dropped: the code builds without it, and it hid
    # non-conforming code that Clang/MSVC then rejected)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-local-typedefs -Werror=array-bounds -Wno-switch -Wwrite-strings -Wno-unknown-warning-option -Wunused-variable")
    # The code assumes x86/MSVC signed char (char tables holding -1, == -1
    # tests); plain char is unsigned on ARM (Raspberry Pi), so pin it.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsigned-char")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsigned-char")
endif()

# Warnings for NEW code. Applied per source file (set_source_files_properties
# ... COMPILE_OPTIONS "${MC2_STRICT_WARNINGS}") in the directory that owns the
# target, so legacy files keep the flags above. GCC/Clang only: on MSVC the
# legacy headers would need /external:I handling to stay quiet.
if(NOT MSVC)
    set(MC2_STRICT_WARNINGS -Wall -Wextra -Werror)
endif()

# MC2_SANITIZE: comma-separated -fsanitize= list for GCC/Clang, e.g.
# "address,undefined". Applied to every target (game, tools, tests).
set(MC2_SANITIZE "" CACHE STRING "Sanitizers for GCC/Clang builds, e.g. address,undefined")
if(MC2_SANITIZE)
    if(MSVC)
        message(FATAL_ERROR "MC2_SANITIZE is only wired for GCC/Clang")
    endif()
    # UBSan reports and continues, so one run lists every finding; the test
    # runners treat any "runtime error:" line as a failure.
    set(_mc2_san "-fsanitize=${MC2_SANITIZE} -fno-omit-frame-pointer")
    # GCC 12/13 report false -Warray-bounds positives under sanitizer
    # instrumentation (e.g. "offset 0 is out of the bounds [0, 0]" on a stack
    # buffer). Normal builds keep -Werror=array-bounds.
    set(_mc2_san "${_mc2_san} -Wno-error=array-bounds")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${_mc2_san}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${_mc2_san}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=${MC2_SANITIZE}")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=${MC2_SANITIZE}")
    message("Sanitizers: ${MC2_SANITIZE}")
endif()
# MC2_FRAME_POINTERS: keep frame pointers so "perf record -g" (scripts/bench.sh
# --perf) can walk call stacks; -O2 drops them. Costs a few percent, so it is
# off for normal builds and benchmark numbers come from a build without it.
option(MC2_FRAME_POINTERS "Keep frame pointers for perf call graphs (GCC/Clang)" OFF)
if(MC2_FRAME_POINTERS)
    if(MSVC)
        message(FATAL_ERROR "MC2_FRAME_POINTERS is only wired for GCC/Clang")
    endif()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer")
    message("Frame pointers: on")
endif()
# -Wno-unused-local-typedefs - to disable "typedef was ignored on this declaration"

if(MSVC)
	add_definitions(-DPLATFORM_WINDOWS)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	message("Compiling on M$ Windows")
endif()

# to support windows sln file generator as well
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_ARMOR -D_DEBUG -DBUGLOG -DLAB_ONLY")
#set(CMAKE_CXX_FLAGS_RELWITHDEBINFO" ${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -DSOME_STUFF")
if(MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd /MP")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP -DWINDOWS_IGNORE_PACKING_MISMATCH")
endif()


add_definitions(-DUSE_ASSEMBLER_CODE=0)
add_definitions(-DLINUX_BUILD)

find_package(SDL2 REQUIRED CONFIG REQUIRED COMPONENTS SDL2)
find_package(SDL2_mixer REQUIRED)
# Android: SDL's Java activity calls SDL_main itself (no SDL2main), and the
# tools that need SDL2_ttf (text_tool) are not built there
if(NOT ANDROID)
    find_package(SDL2 REQUIRED CONFIG REQUIRED COMPONENTS SDL2main)
    find_package(SDL2_ttf REQUIRED)
endif()
find_package(ZLIB REQUIRED)

# MC2_GLES: render through OpenGL ES 3.1 (Raspberry Pi 4/400) instead of
# desktop GL. GLES entry points come straight from libGLESv2, no GLEW.
option(MC2_GLES "Use OpenGL ES 3.1 instead of desktop OpenGL" OFF)
if(MC2_GLES)
    if(ANDROID)
        # the NDK's own libraries (Meta Quest test build)
        set(MC2_GL_LIBS GLESv3 EGL)
    else()
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(GLESV2 REQUIRED IMPORTED_TARGET glesv2)
        set(MC2_GL_LIBS PkgConfig::GLESV2)
    endif()
    add_definitions(-DMC2_GLES)
    message("Rendering backend: OpenGL ES 3.1")
else()
    find_package(GLEW REQUIRED)
    find_package(OpenGL REQUIRED)
    set(MC2_GL_LIBS GLEW::GLEW OpenGL::GL)
endif()

message("Found SDL2 package ${SDL2_LIBRARIES}" )
message("SDL2 prefix: ${SDL2_PREFIX}")
message("OPENGL library found in ${OPENGL}")
message("GLEW library found in ${GLEW}")
message("Zlib library found in ${ZLIB}")

message("OpenGL library linking: ${OPENGL_gl_LIBRARY}")
message("OpenGL library include: ${OPENGL_INCLUDE_DIR}")


get_filename_component(COM_PATH1 "./GameOS/include" ABSOLUTE)
get_filename_component(COM_PATH2 "./GameOS/gameos" ABSOLUTE)

set(COMMON_INCLUDE_DIRS  ${COM_PATH1} ${COM_PATH2})
set(THIRDPARTY_INCLUDE_DIRS "${CMAKE_PREFIX_PATH}/include")
if(ANDROID)
    # the NDK toolchain makes CMAKE_PREFIX_PATH a list: include/ of each entry
    list(TRANSFORM CMAKE_PREFIX_PATH APPEND "/include" OUTPUT_VARIABLE THIRDPARTY_INCLUDE_DIRS)
endif()
set(THIRDPARTY_INTREE_DIR "./thirdparty")

add_subdirectory("./mclib/" "./out/mclib")
add_subdirectory("./mclib/mlr" "./out/mclib/mlr")
add_subdirectory("./mclib/gosfx" "./out/mclib/gosfx")
add_subdirectory("./mclib/stuff" "./out/mclib/stuff")
add_subdirectory("./gui" "./out/gui")
add_subdirectory("./GameOS/gameos" "./out/GameOS/gameos")

add_subdirectory("./GameOS/src" "./out/windows")

set(SOURCES 
    "code/dw.cpp"
    "code/mechcmd2.cpp"
    "code/artlry.cpp"
    "code/bldng.cpp"
    "code/carnage.cpp"
    "code/collsn.cpp"
    "code/comndr.cpp"
    "code/contact.cpp"
    "code/gameobj.cpp"
    "code/gate.cpp"
    "code/goal.cpp"
    "code/group.cpp"
    "code/gvehicl.cpp"
    "code/light.cpp"
    "code/mech.cpp"
    "code/mover.cpp"
    "code/objmgr.cpp"
    "code/objtype.cpp"
    "code/movobj.cpp"
    "code/tacordr.cpp"
    "code/team.cpp"
    "code/terrobj.cpp"
    "code/turret.cpp"
    "code/warrior.cpp"
    "code/weaponbolt.cpp"
    "code/ablmc2.cpp"
    "code/gamecam.cpp"
    "code/mission.cpp"
    "code/mission2.cpp"
    "code/benchscenario.cpp"
    "code/benchdirector.cpp"
    "code/uismokewalk.cpp"
    "code/uismokedriver.cpp"
    "code/objective.cpp"
    "code/saveload.cpp"
    "code/trigger.cpp"
    "code/weather.cpp"
    "gui/aanimobject.cpp"
    "gui/aedit.cpp"
    "code/attributemeter.cpp"
    "code/controlgui.cpp"
    "code/forcegroupbar.cpp"
    "code/gametacmap.cpp"
    "code/infowindow.cpp"
    "code/keyboardref.cpp"
    "code/mechicon.cpp"
    "code/missiongui.cpp"
    "code/mpconnectiontype.cpp"
    "code/mpgamebrowser.cpp"
    "code/mphostgame.cpp"
    "code/mploadmap.cpp"
    "code/mpparameterscreen.cpp"
    "code/mpsetuparea.cpp"
    "code/optionsarea.cpp"
    "code/optionsscreenwrapper.cpp"
    "code/pausewindow.cpp"
    "code/prefs.cpp"
    "code/simplecamera.cpp"
    "code/componentlistbox.cpp"
    "code/loadscreen.cpp"
    "code/logistics.cpp"
    "code/logisticscomponent.cpp"
    "code/logisticsdata.cpp"
    "code/logisticsdialog.cpp"
    "code/logisticsmech.cpp"
    "code/logisticsmechdisplay.cpp"
    "code/logisticsmechicon.cpp"
    "code/logisticsmissioninfo.cpp"
    "code/logisticspilot.cpp"
    "code/logisticspilotlistbox.cpp"
    "gui/logisticsscreen.cpp"
    "code/logisticsvariant.cpp"
    "code/mainmenu.cpp"
    "code/mechbayscreen.cpp"
    "code/mechlabscreen.cpp"
    "code/mechlistbox.cpp"
    "code/mechlopedia.cpp"
    "code/mechpurchasescreen.cpp"
    "code/missionbegin.cpp"
    "code/missionbriefingscreen.cpp"
    "code/missionresults.cpp"
    "code/missionselectionscreen.cpp"
    "code/mpprefs.cpp"
    "code/pilotreadyscreen.cpp"
    "code/pilotreviewarea.cpp"
    "code/salvagemecharea.cpp"
    "code/simplecomponentlistbox.cpp"
    "code/gamesound.cpp"
    "code/radio.cpp"
    "code/movemgr.cpp"
    "code/chatwindow.cpp"
    "code/mpstats.cpp"
    "code/multplyr.cpp"
    "code/mc2movie.cpp"
    )


include_directories(${COMMON_INCLUDE_DIRS} "./mclib" "./gui" "./code" "./netlib")
include_directories(${THIRDPARTY_INCLUDE_DIRS})
include_directories(${THIRDPARTY_INTREE_DIR})

if(NOT WIN32)
	set(ADDITIONAL_LIBS dl)
	message("Not win32 system")
else()
	# winmm for timeGetTime, maybe switch to GetTickCount to remove this dependency
	set(ADDITIONAL_LIBS winmm)
endif()


if(ANDROID)
    # SDL's Java activity loads the game as libmain.so and calls SDL_main
    # (SDL.h renames main) on its own thread; see android/README.md
    add_library(mc2 SHARED ${SOURCES} ${MAIN_SRC})
    set_target_properties(mc2 PROPERTIES OUTPUT_NAME main)
else()
    add_executable(mc2 ${SOURCES} ${MAIN_SRC})
endif()
# new, game-free code: strict warnings. benchdirector.cpp stays on the legacy
# flags: it is glue that includes most of the game's 2001 headers.
# benchscenario.cpp includes inifile.h, whose dstd.h has MSVC-only
# "#pragma warning" lines; the unit tests see it as a SYSTEM header, the game
# target cannot, so only that warning is relaxed here.
if(MC2_STRICT_WARNINGS)
    set_source_files_properties(code/benchscenario.cpp
        PROPERTIES COMPILE_OPTIONS "${MC2_STRICT_WARNINGS};-Wno-unknown-pragmas")
    set_source_files_properties(code/uismokewalk.cpp
        PROPERTIES COMPILE_OPTIONS "${MC2_STRICT_WARNINGS}")
    # uismokedriver.cpp is game glue (like benchdirector.cpp) but new code:
    # the engine's include directories are passed again as -isystem (GCC and
    # Clang then drop the -I), so the strict set reports the driver, not the
    # 2001 headers it includes.
    set(_mc2_legacy_isystem "")
    foreach(_dir GameOS/include GameOS/gameos mclib gui code netlib thirdparty)
        list(APPEND _mc2_legacy_isystem "-isystem" "${CMAKE_CURRENT_SOURCE_DIR}/${_dir}")
    endforeach()
    set_source_files_properties(code/uismokedriver.cpp
        PROPERTIES COMPILE_OPTIONS
        "${MC2_STRICT_WARNINGS};-Wno-unknown-pragmas;${_mc2_legacy_isystem}")
endif()
target_link_libraries(mc2 mclib gosfx mlr stuff gui gameos gameos_main windows ZLIB::ZLIB ${SDL2_LIBRARIES} SDL2_mixer::SDL2_mixer ${ADDITIONAL_LIBS} ${MC2_GL_LIBS})

add_subdirectory("./res" "./out/res")
# Android builds the game only; the data is built on a PC and pushed
if(NOT ANDROID)
    add_subdirectory("./data_tools" "./out/data_tools")
    add_subdirectory("./text_tool" "./out/text_tool")
    add_subdirectory("./Viewer" "./out/Viewer")
endif()

# Unit tests (doctest + CTest): see tests/unit/CMakeLists.txt.
option(MC2_BUILD_TESTS "Build the unit tests (run with ctest)" ON)
if(MC2_BUILD_TESTS)
    enable_testing()
    add_subdirectory("./tests/unit" "./out/tests/unit")
endif()
