set(HDR ${CMAKE_CURRENT_SOURCE_DIR}/include/piga/daemon/sdk)

set(HDRS
    ${HDR}/DBusManager.hpp
    ${HDR}/Plugin.hpp
)

add_library(pigadaemon-sdk STATIC ${HDRS})
set_target_properties(pigadaemon-sdk PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(pigadaemon-sdk PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
    $<INSTALL_INTERFACE:include>
)

export(TARGETS pigadaemon-sdk APPEND FILE PigaDaemonConfig.cmake)

include(GNUInstallDirs)

install(TARGETS pigadaemon-sdk
    EXPORT PigaDaemonConfig
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})  # This is for Windows
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
