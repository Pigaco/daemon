cmake_minimum_required(VERSION 2.6)
project(devkit)
cmake_policy(SET CMP0022 NEW)

set(DEVKIT_VERSION_MAJOR "0" CACHE STRING "Major-Version")
set(DEVKIT_VERSION_MINOR "1" CACHE STRING "Minor-Version")
set(DEVKIT_VERSION_PATCH "3" CACHE STRING "Patch-Version")
set(DEVKIT_VERSION_TWEAK "0" CACHE STRING "Tweak-Version")
set(DEVKIT_VERSION_BUILD "0" CACHE STRING "Build")
set(DEVKIT_VERSION ${DEVKIT_VERSION_MAJOR}.${DEVKIT_VERSION_MINOR}.${DEVKIT_VERSION_PATCH}.${DEVKIT_VERSION_TWEAK}:${DEVKIT_VERSION_BUILD})

set(SRC ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(SRCS 
    ${SRC}/Devkit.cpp
    ${SRC}/HTTPServer.cpp
    ${SRC}/ExportsManager.cpp
    ${SRC}/WebUI.cpp
)

add_library(devkit SHARED ${SRCS})

target_include_directories(devkit PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_include_directories(devkit PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../third-party/include/>
)

find_package(Boost COMPONENTS system filesystem program_options log REQUIRED)
if(${Boost_FOUND})
    target_include_directories(devkit PRIVATE ${Boost_INCLUDE_DIR})
    target_link_libraries(devkit PUBLIC ${Boost_LIBRARIES})
    add_definitions("-DHAS_BOOST -DBOOST_LOG_DYN_LINK")
endif()

# http://stackoverflow.com/a/25836953 (Backwards compatible C++11 declaration.)
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++14" COMPILER_SUPPORTS_CXX14)
if(COMPILER_SUPPORTS_CXX14)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
else()
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++14 support. Please use a different C++ compiler.")
endif()

find_package(MHD REQUIRED) 
target_link_libraries(devkit PRIVATE ${MHD_LIBRARIES})
target_include_directories(devkit PRIVATE ${MHD_INCLUDE_DIR})

target_link_libraries(devkit PUBLIC pigadaemon-sdk)

find_package(Lua EXACT 5.2 REQUIRED)
target_link_libraries(devkit PRIVATE ${LUA_LIBRARIES} dl)
target_include_directories(devkit PRIVATE ${LUA_INCLUDE_DIR})

include(GNUInstallDirs)

install(TARGETS devkit EXPORT PigaDaemonConfig
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/web
    DESTINATION /usr/share/piga/devkit/)
