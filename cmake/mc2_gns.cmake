# The network transport's library: GameNetworkingSockets (GNS), its protobuf and
# its crypto, built from pinned sources as separate projects and linked
# statically (planning/future-features/NETWORKING.md, Decision 4).
#
# Separate projects, not add_subdirectory: the game's global flags (-std=c++0x,
# -fsigned-char, -Werror=array-bounds, sanitizers) are for the game's own code.
# They always build as Release. Sources are downloaded at build time, once per
# build directory; -DMC2_NET=OFF builds the game without any of this.
#
# Crypto: BoringSSL, which needs no Perl and so builds the same on Linux, Quest and
# from Git Bash (GNS needs a small patch for it: mc2_gns_boringssl_patch.cmake); on
# Windows, GNS's own BCrypt backend, part of the system.
#
# Gives an interface target, mc2_gns, for what uses GNS (GameOS/gameos/gos_net_gns.cpp).

include(ExternalProject)

set(_gns_prefix "${CMAKE_BINARY_DIR}/mc2_gns")
set(_gns_lib "${_gns_prefix}/lib")
set(_gns_include "${_gns_prefix}/include/GameNetworkingSockets")
file(MAKE_DIRECTORY "${_gns_include}")

set(_gns_args
    -DCMAKE_BUILD_TYPE=Release
    "-DCMAKE_INSTALL_PREFIX=${_gns_prefix}"
    -DCMAKE_INSTALL_LIBDIR=lib
    "-DCMAKE_PREFIX_PATH=${_gns_prefix}"
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5)
if(NOT MSVC)
    list(APPEND _gns_args
        "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}" "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
        "-DCMAKE_C_COMPILER_LAUNCHER=${CMAKE_C_COMPILER_LAUNCHER}"
        "-DCMAKE_CXX_COMPILER_LAUNCHER=${CMAKE_CXX_COMPILER_LAUNCHER}"
        "-DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}")
endif()
if(ANDROID)
    list(APPEND _gns_args
        "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}"
        "-DANDROID_ABI=${ANDROID_ABI}" "-DANDROID_PLATFORM=${ANDROID_PLATFORM}"
        "-DCMAKE_FIND_ROOT_PATH=${_gns_prefix}")
endif()

# One configuration, whatever the generator
set(_gns_build ${CMAKE_COMMAND} --build <BINARY_DIR> --config Release)
set(_gns_install ${CMAKE_COMMAND} --install <BINARY_DIR> --config Release)

if(MSVC)
    set(_gns_libs "${_gns_lib}/GameNetworkingSockets_s.lib" "${_gns_lib}/libprotobuf.lib")
else()
    set(_gns_libs "${_gns_lib}/libGameNetworkingSockets_s.a" "${_gns_lib}/libprotobuf.a")
endif()

# protobuf 21.12: the last before Abseil. Cross-compiling needs a protoc for the
# machine doing the build, which comes prebuilt.
set(_gns_protobuf_args -Dprotobuf_BUILD_TESTS=OFF -Dprotobuf_WITH_ZLIB=OFF
    -Dprotobuf_BUILD_SHARED_LIBS=OFF -Dprotobuf_MSVC_STATIC_RUNTIME=OFF)
if(CMAKE_CROSSCOMPILING)
    list(APPEND _gns_protobuf_args -Dprotobuf_BUILD_PROTOC_BINARIES=OFF)
    if(CMAKE_HOST_WIN32)
        set(_gns_protoc_url https://github.com/protocolbuffers/protobuf/releases/download/v21.12/protoc-21.12-win64.zip)
        set(_gns_protoc_hash 71852a30cf62975358edfcbbff93086e8857a079c8e4d6904881aa968d65c7f9)
        set(_gns_protoc_exe protoc.exe)
    else()
        set(_gns_protoc_url https://github.com/protocolbuffers/protobuf/releases/download/v21.12/protoc-21.12-linux-x86_64.zip)
        set(_gns_protoc_hash 3a4c1e5f2516c639d3079b1586e703fc7bcfa2136d58bda24d1d54f949c315e8)
        set(_gns_protoc_exe protoc)
    endif()
    ExternalProject_Add(mc2_gns_protoc
        URL ${_gns_protoc_url} URL_HASH SHA256=${_gns_protoc_hash}
        DOWNLOAD_NO_EXTRACT FALSE
        PREFIX "${_gns_prefix}/protoc"
        CONFIGURE_COMMAND "" BUILD_COMMAND "" INSTALL_COMMAND "")
    ExternalProject_Get_Property(mc2_gns_protoc SOURCE_DIR)
    set(_gns_protoc "${SOURCE_DIR}/bin/${_gns_protoc_exe}")
    set(_gns_host_protoc -DProtobuf_PROTOC_EXECUTABLE=${_gns_protoc} -DGNS_HOST_PROTOC=${_gns_protoc})
    set(_gns_protoc_dep mc2_gns_protoc)
endif()

ExternalProject_Add(mc2_gns_protobuf
    URL https://github.com/protocolbuffers/protobuf/releases/download/v21.12/protobuf-cpp-3.21.12.tar.gz
    URL_HASH SHA256=4eab9b524aa5913c6fffb20b2a8abf5ef7f95a80bc0701f3a6dbb4c607f73460
    PREFIX "${_gns_prefix}/protobuf"
    CMAKE_ARGS ${_gns_args} ${_gns_protobuf_args}
    BUILD_COMMAND ${_gns_build}
    INSTALL_COMMAND ${_gns_install}
    BUILD_BYPRODUCTS ${_gns_libs})

if(MSVC)
    set(_gns_crypto -DUSE_CRYPTO=BCrypt)
    set(_gns_crypto_dep "")
else()
    # only libcrypto, and only by hand: its install rule wants everything built
    set(_gns_crypto_lib "${_gns_lib}/libcrypto.a")
    ExternalProject_Add(mc2_gns_boringssl
        URL https://github.com/google/boringssl/archive/refs/tags/0.20260929.0.tar.gz
        URL_HASH SHA256=04da9ba0664e0a7f028e961c38d604f2cc6dac852a84ca0e51dc1fa051d4c8fe
        PREFIX "${_gns_prefix}/boringssl"
        CMAKE_ARGS ${_gns_args} -DBUILD_TESTING=OFF
        BUILD_COMMAND ${_gns_build} --target crypto
        INSTALL_COMMAND ${CMAKE_COMMAND} -E make_directory "${_gns_lib}" "${_gns_prefix}/include"
        COMMAND ${CMAKE_COMMAND} -E copy <BINARY_DIR>/libcrypto.a "${_gns_crypto_lib}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/include/openssl "${_gns_prefix}/include/openssl"
        BUILD_BYPRODUCTS "${_gns_crypto_lib}")
    # GNS's checks link through FindOpenSSL, which can pull in a system OpenSSL's
    # pkg-config flags and so fail. BoringSSL has both functions; linking proves it.
    set(_gns_crypto -DUSE_CRYPTO=OpenSSL "-DOPENSSL_ROOT_DIR=${_gns_prefix}"
        -DOPENSSL_NEW_ENOUGH=ON -DOPENSSL_HAS_25519_RAW=ON)
    set(_gns_crypto_dep mc2_gns_boringssl)
    set(_gns_patch ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_LIST_DIR}/mc2_gns_boringssl_patch.cmake")
    list(APPEND _gns_libs "${_gns_crypto_lib}")
endif()

ExternalProject_Add(mc2_gns_library
    URL https://github.com/ValveSoftware/GameNetworkingSockets/archive/refs/tags/v1.6.1.tar.gz
    URL_HASH SHA256=4091ddcb5285134d5d0e97385ae4792b828e5b103c8ae2c79652dc7d177366d9
    PREFIX "${_gns_prefix}/gns"
    DEPENDS mc2_gns_protobuf ${_gns_crypto_dep} ${_gns_protoc_dep}
    PATCH_COMMAND ${_gns_patch}
    CMAKE_ARGS ${_gns_args} ${_gns_crypto} ${_gns_host_protoc}
        -DBUILD_STATIC_LIB=ON -DBUILD_SHARED_LIB=OFF -DENABLE_ICE=ON
        -DProtobuf_USE_STATIC_LIBS=ON
    BUILD_COMMAND ${_gns_build}
    INSTALL_COMMAND ${_gns_install})

add_library(mc2_gns INTERFACE)
add_dependencies(mc2_gns mc2_gns_library)
target_include_directories(mc2_gns SYSTEM INTERFACE "${_gns_include}")
target_compile_definitions(mc2_gns INTERFACE STEAMNETWORKINGSOCKETS_STATIC_LINK)
target_link_libraries(mc2_gns INTERFACE ${_gns_libs})
if(WIN32)
    target_link_libraries(mc2_gns INTERFACE ws2_32 iphlpapi crypt32 bcrypt winmm)
elseif(ANDROID)
    target_link_libraries(mc2_gns INTERFACE log)
else()
    find_package(Threads REQUIRED)
    target_link_libraries(mc2_gns INTERFACE Threads::Threads)
endif()
