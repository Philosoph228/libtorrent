cmake_minimum_required(VERSION 3.20)

project(libtorrent VERSION 0.16.8 LANGUAGES C CXX)

include(GNUInstallDirs)

option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(LIBTORRENT_BUILD_TESTS "Build libtorrent tests" OFF)
option(LIBTORRENT_ENABLE_INSTRUMENTATION "Enable instrumentation hooks" ON)
option(LIBTORRENT_ENABLE_EXTRA_DEBUG "Enable extra debug checks" OFF)
option(LIBTORRENT_USE_ALIGNED "Enable alignment-safe code paths" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)
find_package(ZLIB REQUIRED)
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)

include(CheckIncludeFile)
include(CheckSymbolExists)
include(CheckCXXCompilerFlag)
include(CheckCXXSourceCompiles)
include(CheckTypeSize)
include(TestBigEndian)

check_include_file("sys/vfs.h" HAVE_SYS_VFS_H)
check_include_file("sys/statvfs.h" HAVE_SYS_STATVFS_H)
check_include_file("sys/statfs.h" HAVE_SYS_STATFS_H)

check_symbol_exists(backtrace "execinfo.h" HAVE_BACKTRACE)

check_cxx_source_compiles(
  "int main() { return __builtin_popcount(0); }"
  USE_BUILTIN_POPCOUNT
)

if(WIN32)
  set(USE_EPOLL 0)
  set(USE_KQUEUE 0)
else()
  check_cxx_source_compiles(
    "#include <sys/epoll.h>\nint main() { int fd = epoll_create(100); return fd; }"
    USE_EPOLL
  )

  check_cxx_source_compiles(
    "#include <sys/time.h>\n#include <sys/event.h>\nint main() { int fd = kqueue(); return fd; }"
    USE_KQUEUE
  )

  if(NOT USE_EPOLL AND NOT USE_KQUEUE)
    message(FATAL_ERROR "Neither epoll nor kqueue are available. libtorrent requires one of them.")
  endif()
endif()

check_cxx_source_compiles(
  "#include <sys/inotify.h>\nint main() { return inotify_init() == -1; }"
  USE_INOTIFY
)

check_cxx_source_compiles(
  "#define _GNU_SOURCE\n#include <fcntl.h>\nint main() { return fallocate(0, FALLOC_FL_KEEP_SIZE, 0, 0); }"
  USE_FALLOCATE
)

check_cxx_source_compiles(
  "#include <fcntl.h>\nint main() { return posix_fallocate(0, 0, 0); }"
  USE_POSIX_FALLOCATE
)

check_cxx_source_compiles(
  "#include <fcntl.h>\nint main() { return posix_fadvise(0, 0, 0, POSIX_FADV_RANDOM); }"
  USE_POSIX_FADVISE
)

check_cxx_source_compiles(
  "#include <sys/mman.h>\nint main() { char test[1024]; return madvise(test, sizeof(test), MADV_NORMAL); }"
  USE_MADVISE
)

check_cxx_source_compiles(
  "#include <sys/mman.h>\n#include <unistd.h>\nint main() { mincore((char*)0, 0, (unsigned char*)0); return 0; }"
  HAVE_MINCORE_UNSIGNED
)

if(HAVE_MINCORE_UNSIGNED)
  set(USE_MINCORE 1)
  set(USE_MINCORE_UNSIGNED 1)
else()
  check_cxx_source_compiles(
    "#include <sys/mman.h>\n#include <unistd.h>\nint main() { mincore((char*)0, 0, (char*)0); return 0; }"
    HAVE_MINCORE_SIGNED
  )
  if(HAVE_MINCORE_SIGNED)
    set(USE_MINCORE 1)
  endif()
endif()

set(CMAKE_REQUIRED_LIBRARIES "${CMAKE_THREAD_LIBS_INIT}")
check_cxx_source_compiles(
  "#include <pthread.h>\nint main() { pthread_t t; pthread_setname_np(t, \"foo\"); return 0; }"
  HAS_PTHREAD_SETNAME_NP_GENERIC
)
check_cxx_source_compiles(
  "#include <pthread.h>\nint main() { pthread_setname_np(\"foo\"); return 0; }"
  HAS_PTHREAD_SETNAME_NP_DARWIN
)
set(CMAKE_REQUIRED_LIBRARIES "")

check_cxx_compiler_flag("-fvisibility=hidden" SUPPORT_ATTRIBUTE_VISIBILITY)

if(SUPPORT_ATTRIBUTE_VISIBILITY)
  add_compile_options(-fvisibility=hidden)
endif()

if(APPLE)
  set(SYS_DARWIN 1)
endif()

if(WIN32)
  set(WINDOWS 1)
endif()

if(LIBTORRENT_ENABLE_INSTRUMENTATION)
  set(LT_INSTRUMENTATION 1)
endif()

if(LIBTORRENT_ENABLE_EXTRA_DEBUG)
  set(USE_EXTRA_DEBUG 1)
endif()

if(LIBTORRENT_USE_ALIGNED)
  set(USE_ALIGNED 1)
endif()

test_big_endian(IS_BIG_ENDIAN)
if(IS_BIG_ENDIAN)
  set(IS_BIG_ENDIAN 1)
else()
  set(IS_LITTLE_ENDIAN 1)
endif()

check_type_size("long" SIZEOF_LONG)
if(SIZEOF_LONG EQUAL 8)
  set(DEFAULT_ADDRESS_SPACE_SIZE 4096)
else()
  set(DEFAULT_ADDRESS_SPACE_SIZE 1024)
endif()

set(LT_SMP_CACHE_BYTES 128)
set(PEER_NAME "-lt1008-")
set(PEER_VERSION "lt\\x10\\x08")

check_symbol_exists(statvfs "sys/statvfs.h" HAVE_STATVFS)
check_symbol_exists(statfs "sys/statfs.h;sys/vfs.h" HAVE_STATFS)

if(HAVE_STATVFS)
  set(FS_STAT_STRUCT "struct statvfs")
  set(FS_STAT_FN "statvfs(fn, &m_stat) == 0")
  set(FS_STAT_FD "fstatvfs(fd, &m_stat) == 0")
  set(FS_STAT_SIZE_TYPE "unsigned long")
  set(FS_STAT_COUNT_TYPE "fsblkcnt_t")
  set(FS_STAT_BLOCK_SIZE "(m_stat.f_frsize)")
elseif(HAVE_STATFS)
  set(FS_STAT_STRUCT "struct statfs")
  set(FS_STAT_FN "statfs(fn, &m_stat) == 0")
  set(FS_STAT_FD "fstatfs(fd, &m_stat) == 0")
  set(FS_STAT_SIZE_TYPE "long")
  set(FS_STAT_COUNT_TYPE "long")
  set(FS_STAT_BLOCK_SIZE "(m_stat.f_bsize)")
else()
  set(FS_STAT_STRUCT "struct { int f_bsize; int f_bavail; }")
  set(FS_STAT_FN "(errno = ENOSYS) == 0")
  set(FS_STAT_FD "(errno = ENOSYS) == 0")
  set(FS_STAT_SIZE_TYPE "int")
  set(FS_STAT_COUNT_TYPE "int")
  set(FS_STAT_BLOCK_SIZE "(4096)")
endif()

set(_atomic_test_src
  "#include <atomic>\nint main() { std::atomic<long long> v{0}; return (int)v.load(); }"
)
set(LIBTORRENT_ATOMIC_LIBS "")
check_cxx_source_compiles("${_atomic_test_src}" HAVE_BUILTIN_ATOMICS)
if(NOT HAVE_BUILTIN_ATOMICS)
  set(CMAKE_REQUIRED_LIBRARIES atomic)
  check_cxx_source_compiles("${_atomic_test_src}" HAVE_ATOMIC_LIB)
  if(HAVE_ATOMIC_LIB)
    set(LIBTORRENT_ATOMIC_LIBS atomic)
  endif()
  set(CMAKE_REQUIRED_LIBRARIES "")
endif()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/config.h"
)

file(GLOB_RECURSE LIBTORRENT_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc"
)

add_library(torrent ${LIBTORRENT_SOURCES})
add_library(libtorrent ALIAS torrent)

if(TARGET CURL::libcurl)
  set(LIBTORRENT_CURL_TARGET CURL::libcurl)
elseif(TARGET CURL::CURL)
  set(LIBTORRENT_CURL_TARGET CURL::CURL)
else()
  message(FATAL_ERROR "CURL target not found by find_package(CURL).")
endif()

target_include_directories(torrent
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if(UNIX AND NOT WIN32)
  target_compile_definitions(torrent PUBLIC _FILE_OFFSET_BITS=64)
endif()

target_link_libraries(torrent
  PUBLIC
    Threads::Threads
    ZLIB::ZLIB
    ${LIBTORRENT_CURL_TARGET}
    OpenSSL::Crypto
    ${LIBTORRENT_ATOMIC_LIBS}
)

if(WIN32)
  target_link_libraries(torrent PUBLIC ws2_32)
endif()

set(LIBTORRENT_CURRENT 38)
set(LIBTORRENT_REVISION 0)
set(LIBTORRENT_AGE 0)
set(LIBTORRENT_INTERFACE_VERSION_NO
  "${LIBTORRENT_CURRENT}.${LIBTORRENT_AGE}.${LIBTORRENT_REVISION}"
)

set_target_properties(torrent PROPERTIES
  VERSION "${LIBTORRENT_INTERFACE_VERSION_NO}"
  SOVERSION "${LIBTORRENT_CURRENT}"
)

set(prefix "${CMAKE_INSTALL_PREFIX}")
set(exec_prefix "${CMAKE_INSTALL_PREFIX}")
set(libdir "${CMAKE_INSTALL_FULL_LIBDIR}")
set(includedir "${CMAKE_INSTALL_FULL_INCLUDEDIR}")
set(VERSION "${PROJECT_VERSION}")
set(PTHREAD_LIBS "${CMAKE_THREAD_LIBS_INIT}")
set(ATOMIC_LIBS "")
if(LIBTORRENT_ATOMIC_LIBS)
  set(ATOMIC_LIBS "-latomic")
endif()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/libtorrent.pc.in"
  "${CMAKE_CURRENT_BINARY_DIR}/libtorrent.pc"
  @ONLY
)

install(TARGETS torrent
  EXPORT libtorrentTargets
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src/torrent/"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/torrent"
  FILES_MATCHING PATTERN "*.h"
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libtorrent.pc"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
)

if(LIBTORRENT_BUILD_TESTS)
  enable_testing()
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(CPPUNIT REQUIRED IMPORTED_TARGET cppunit)

  set(LIBTORRENT_TEST_COMMON
    test/main.cc
    test/helpers/expect_fd.h
    test/helpers/expect_utils.h
    test/helpers/mock_compare.h
    test/helpers/mock_function.cc
    test/helpers/mock_function.h
    test/helpers/network.cc
    test/helpers/network.h
    test/helpers/progress_listener.cc
    test/helpers/progress_listener.h
    test/helpers/protectors.cc
    test/helpers/protectors.h
    test/helpers/test_fixture.cc
    test/helpers/test_fixture.h
    test/helpers/test_main_thread.cc
    test/helpers/test_main_thread.h
    test/helpers/test_thread.cc
    test/helpers/test_thread.h
    test/helpers/test_utils.h
    test/helpers/tracker_test.cc
    test/helpers/tracker_test.h
    test/helpers/utils.h
  )

  set(LIBTORRENT_TEST_TORRENT_NET_SOURCES
    ${LIBTORRENT_TEST_COMMON}
    test/torrent/net/test_address_info.cc
    test/torrent/net/test_address_info.h
    test/torrent/net/test_fd.cc
    test/torrent/net/test_fd.h
    test/torrent/net/test_socket_address.cc
    test/torrent/net/test_socket_address.h
  )

  set(LIBTORRENT_TEST_TORRENT_UTILS_SOURCES
    ${LIBTORRENT_TEST_COMMON}
    test/torrent/utils/test_extents.cc
    test/torrent/utils/test_extents.h
    test/torrent/utils/test_log.cc
    test/torrent/utils/test_log.h
    test/torrent/utils/test_log_buffer.cc
    test/torrent/utils/test_log_buffer.h
    test/torrent/utils/test_option_strings.cc
    test/torrent/utils/test_option_strings.h
    test/torrent/utils/test_queue_buckets.cc
    test/torrent/utils/test_queue_buckets.h
    test/torrent/utils/test_signal_bitfield.cc
    test/torrent/utils/test_signal_bitfield.h
    test/torrent/utils/test_signal_interrupt.cc
    test/torrent/utils/test_signal_interrupt.h
    test/torrent/utils/test_thread_base.cc
    test/torrent/utils/test_thread_base.h
    test/torrent/utils/test_uri_parser.cc
    test/torrent/utils/test_uri_parser.h
  )

  set(LIBTORRENT_TEST_TORRENT_SOURCES
    ${LIBTORRENT_TEST_COMMON}
    test/torrent/object_test.cc
    test/torrent/object_test.h
    test/torrent/object_test_utils.cc
    test/torrent/object_test_utils.h
    test/torrent/object_static_map_test.cc
    test/torrent/object_static_map_test.h
    test/torrent/object_stream_test.cc
    test/torrent/object_stream_test.h
    test/torrent/test_tracker_controller.cc
    test/torrent/test_tracker_controller.h
    test/torrent/test_tracker_controller_features.cc
    test/torrent/test_tracker_controller_features.h
    test/torrent/test_tracker_controller_requesting.cc
    test/torrent/test_tracker_controller_requesting.h
    test/torrent/test_tracker_list.cc
    test/torrent/test_tracker_list.h
    test/torrent/test_tracker_list_features.cc
    test/torrent/test_tracker_list_features.h
    test/torrent/test_tracker_timeout.cc
    test/torrent/test_tracker_timeout.h
  )

  set(LIBTORRENT_TEST_DATA_SOURCES
    ${LIBTORRENT_TEST_COMMON}
    test/data/test_chunk_list.cc
    test/data/test_chunk_list.h
    test/data/test_hash_check_queue.cc
    test/data/test_hash_check_queue.h
    test/data/test_hash_queue.cc
    test/data/test_hash_queue.h
  )

  set(LIBTORRENT_TEST_NET_SOURCES
    ${LIBTORRENT_TEST_COMMON}
  )

  set(LIBTORRENT_TEST_TRACKER_SOURCES
    ${LIBTORRENT_TEST_COMMON}
    test/tracker/test_tracker_http.cc
    test/tracker/test_tracker_http.h
  )

  set(LIBTORRENT_TEST_SOURCES
    ${LIBTORRENT_TEST_COMMON}
    test/rak/allocators_test.cc
    test/rak/allocators_test.h
    test/rak/ranges_test.cc
    test/rak/ranges_test.h
    test/protocol/test_request_list.cc
    test/protocol/test_request_list.h
  )

  add_executable(LibTorrent_Test_Torrent_Net ${LIBTORRENT_TEST_TORRENT_NET_SOURCES})
  add_executable(LibTorrent_Test_Torrent_Utils ${LIBTORRENT_TEST_TORRENT_UTILS_SOURCES})
  add_executable(LibTorrent_Test_Torrent ${LIBTORRENT_TEST_TORRENT_SOURCES})
  add_executable(LibTorrent_Test_Data ${LIBTORRENT_TEST_DATA_SOURCES})
  add_executable(LibTorrent_Test_Net ${LIBTORRENT_TEST_NET_SOURCES})
  add_executable(LibTorrent_Test_Tracker ${LIBTORRENT_TEST_TRACKER_SOURCES})
  add_executable(LibTorrent_Test ${LIBTORRENT_TEST_SOURCES})

  set(LIBTORRENT_TEST_TARGETS
    LibTorrent_Test_Torrent_Net
    LibTorrent_Test_Torrent_Utils
    LibTorrent_Test_Torrent
    LibTorrent_Test_Data
    LibTorrent_Test_Net
    LibTorrent_Test_Tracker
    LibTorrent_Test
  )

  foreach(test_target IN LISTS LIBTORRENT_TEST_TARGETS)
    target_include_directories(${test_target}
      PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/test"
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_BINARY_DIR}"
        ${CPPUNIT_INCLUDE_DIRS}
    )
    target_link_libraries(${test_target}
      PRIVATE
        torrent
        PkgConfig::CPPUNIT
    )
    add_test(NAME ${test_target} COMMAND ${test_target})
  endforeach()
endif()
