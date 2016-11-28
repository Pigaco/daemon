project(piga-daemon C CXX)
cmake_minimum_required(VERSION 3.0.1)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Library Version
set(DAEMON_VERSION_MAJOR "0" CACHE STRING "Major-Version")
set(DAEMON_VERSION_MINOR "9" CACHE STRING "Minor-Version")
set(DAEMON_VERSION_PATCH "3" CACHE STRING "Patch-Version")
set(DAEMON_VERSION_TWEAK "0" CACHE STRING "Tweak-Version")
set(DAEMON_VERSION_BUILD "0" CACHE STRING "Build")
set(DAEMON_VERSION ${DAEMON_VERSION_MAJOR}.${DAEMON_VERSION_MINOR}.${DAEMON_VERSION_PATCH}.${DAEMON_VERSION_TWEAK}:${DAEMON_VERSION_BUILD})

set(HDR ${CMAKE_CURRENT_SOURCE_DIR}/include/piga/daemon)
set(SRC ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(HDRS
    ${HDR}/Daemon.hpp
    ${HDR}/Host.hpp
    ${HDR}/Loader.hpp
    ${HDR}/App.hpp
    ${HDR}/AppManager.hpp
    ${HDR}/PluginManager.hpp
    ${HDR}/DBusManager.hpp
    ${HDR}/LogManager.hpp
)
set(SRCS
    ${SRC}/Daemon.cpp
    ${SRC}/Host.cpp
    ${SRC}/Loader.cpp
    ${SRC}/App.cpp
    ${SRC}/AppManager.cpp
    ${SRC}/PluginManager.cpp
    ${SRC}/DBusManager.cpp
    ${SRC}/LogManager.cpp
)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)

add_library(piga_daemon SHARED ${HDRS} ${SRCS})

export(TARGETS piga_daemon FILE PigaDaemonConfig.cmake)

include(GNUInstallDirs)

install(TARGETS piga_daemon
    EXPORT PigaDaemonConfig
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})  # This is for Windows
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT PigaDaemonConfig DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake)

# Install the service file.
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/piga-daemon.service DESTINATION /etc/systemd/system/)

add_executable(pigadaemon
    ${SRC}/main.cpp)
    
install(TARGETS pigadaemon
    EXPORT PigaDaemonConfig
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})  # This is for Windows

target_link_libraries(pigadaemon piga_daemon)

# http://stackoverflow.com/a/25836953 (Backwards compatible C++11 declaration.)
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
        message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

target_include_directories(piga_daemon PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

find_package(Boost COMPONENTS system filesystem program_options serialization log_setup log REQUIRED)
if(${Boost_FOUND})
    target_include_directories(piga_daemon PRIVATE ${Boost_INCLUDE_DIR})
    target_link_libraries(piga_daemon ${Boost_LIBRARIES})
    add_definitions("-DHAS_BOOST -DBOOST_LOG_DYN_LINK")
endif()

find_package(Threads REQUIRED)
target_link_libraries(piga_daemon ${CMAKE_THREADS_LIBS_INIT})
target_link_libraries(piga_daemon pthread)

target_link_libraries(piga_daemon ${CMAKE_DL_LIBS})

find_package(Config++ REQUIRED)
target_link_libraries(piga_daemon ${CONFIG++_LIBRARY})
target_include_directories(piga_daemon PRIVATE ${CONFIG++_INCLUDE_DIR})

find_package(Systemd REQUIRED)
target_link_libraries(piga_daemon ${SYSTEMD_LIBRARIES})
target_include_directories(piga_daemon PRIVATE ${SYSTEMD_INCLUDE_DIRS})

find_package(LibpigaHosts REQUIRED)
if(${LibpigaHosts_FOUND})
    include_directories(${LibpigaHosts_INCLUDE_DIRS})
endif()

find_package(Piga REQUIRED)
if(${Piga_FOUND})
    target_link_libraries(piga_daemon ${PIGA_COMMON_LIBRARY} ${PIGA_SERVER_LIBRARY} ${PIGA_CLIENT_LIBRARY})
    include_directories(${PIGA_INCLUDE_DIRS})
endif()

add_subdirectory(plugin-sdk)
target_link_libraries(piga_daemon pigadaemon-sdk)

add_subdirectory(devkit)
export(TARGETS devkit APPEND FILE PigaDaemonConfig.cmake)
target_link_libraries(piga_daemon devkit)

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "The main daemon that starts and stops applications, loads host files and listens for inputs in the piga system.")
set(CPACK_PACKAGE_VENDOR "Pigaco")
set(CPACK_PACKAGE_VERSION_MAJOR ${DAEMON_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${DAEMON_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${DAEMON_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION_TWEAK ${DAEMON_VERSION_TWEAK})
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "http://pigaco.net/pages/pigadaemon.html")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "piga-libpiga (>= 0.9.5), libconfig++9v5, libmicrohttpd12, nfs-kernel-server")
set(CPACK_PACKAGE_NAME "piga-daemon")
set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Max Heisinger <mail@maximaximal.com>")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/piga-daemon.service.in ${CMAKE_CURRENT_BINARY_DIR}/piga-daemon.service)

include(CPack)
