project(piga-daemon)
cmake_minimum_required(VERSION 3.0.1)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Library Version
set(DAEMON_VERSION_MAJOR "0" CACHE STRING "Major-Version")
set(DAEMON_VERSION_MINOR "8" CACHE STRING "Minor-Version")
set(DAEMON_VERSION_PATCH "1" CACHE STRING "Patch-Version")
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
)
set(SRCS
    ${SRC}/Daemon.cpp
    ${SRC}/Host.cpp
    ${SRC}/Loader.cpp
    ${SRC}/App.cpp
    ${SRC}/AppManager.cpp
)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)

add_library(piga_daemon SHARED ${HDRS} ${SRCS})

export(TARGETS piga_daemon FILE PigaDaemonConfig.cmake)

install(TARGETS piga_daemon
    EXPORT PigaDaemonConfig
    ARCHIVE  DESTINATION lib
    LIBRARY  DESTINATION lib
    RUNTIME  DESTINATION bin)  # This is for Windows
install(DIRECTORY include/ DESTINATION include)
install(EXPORT PigaDaemonConfig DESTINATION lib/cmake)

# Install the service file.
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/piga-daemon.service DESTINATION etc/systemd/system/)

add_executable(pigadaemon
    ${SRC}/main.cpp)

target_link_libraries(pigadaemon piga_daemon)
set_property(TARGET pigadaemon PROPERTY COMPILE_FLAGS "-std=c++11")

target_include_directories(piga_daemon PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

find_package(Boost COMPONENTS system filesystem program_options serialization log REQUIRED)
if(${Boost_FOUND})
    target_include_directories(piga_daemon PRIVATE ${Boost_INCLUDE_DIR})
    target_link_libraries(piga_daemon ${Boost_LIBRARIES})
    add_definitions("-DHAS_BOOST -DBOOST_LOG_DYN_LINK")
endif()

find_package(Threads REQUIRED)
target_link_libraries(piga_daemon ${CMAKE_THREADS_LIBS_INIT})
if(CMAKE_USE_PTHREADS_INIT)
    set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-lpthread")
    set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-pthread")
endif()

find_package(Config++ REQUIRED)
target_link_libraries(piga_daemon ${CONFIG++_LIBRARY})
target_include_directories(piga_daemon PRIVATE ${CONFIG++_INCLUDE_DIR})

find_package(LibpigaHosts REQUIRED)
if(${LibpigaHosts_FOUND})
    include_directories(${LibpigaHosts_INCLUDE_DIRS})
endif()

find_package(Piga REQUIRED)
if(${Piga_FOUND})
    target_link_libraries(piga_daemon ${PIGA_COMMON_LIBRARY} ${PIGA_SERVER_LIBRARY} ${PIGA_CLIENT_LIBRARY})
    include_directories(${PIGA_INCLUDE_DIRS})
endif()

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "The main daemon that starts and stops applications, loads host files and listens for inputs in the piga system.")
set(CPACK_PACKAGE_VENDOR "Pigaco")
set(CPACK_PACKAGE_VERSION_MAJOR ${DAEMON_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${DAEMON_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${DAEMON_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION_TWEAK ${DAEMON_VERSION_TWEAK})
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "http://pigaco.net/pages/pigadaemon.html")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "piga-libpiga (>= 0.9.5), libconfig++9 (>= 1.4.9), libboost-system1.55.0, libboost-filesystem1.55.0, libboost-program-options1.55.0, libboost-serialization1.55.0, libboost-log1.55.0")
set(CPACK_PACKAGE_NAME "piga-daemon")
set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Max Heisinger <mail@maximaximal.com>")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/piga-daemon.service.in ${CMAKE_CURRENT_BINARY_DIR}/piga-daemon.service)

include(CPack)
