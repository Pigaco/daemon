project(piga-daemon)
cmake_minimum_required(VERSION 3.0.1)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Library Version
set(DAEMON_VERSION_MAJOR "0" CACHE STRING "Major-Version")
set(DAEMON_VERSION_MINOR "8" CACHE STRING "Minor-Version")
set(DAEMON_VERSION_PATCH "0" CACHE STRING "Patch-Version")
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

target_compile_features(piga_daemon PRIVATE cxx_nullptr)

add_executable(pigadaemon
    ${SRC}/main.cpp)

target_compile_features(pigadaemon PRIVATE cxx_nullptr)

target_link_libraries(pigadaemon piga_daemon)

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
