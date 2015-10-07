cmake_minimum_required (VERSION 2.6)
set(CMAKE_VERBOSE_MAKEFILE true)

project (DownloadDaemon)
add_subdirectory (daemon)
#add_subdirectory (lib)
add_subdirectory (daemon/plugins)

project (ddconsole)
add_subdirectory (ddconsole)
#add_subdirectory (lib)

#project (ddclient-wx)
#add_subdirectory (ddclient-wx)
#add_subdirectory (lib)

project (ddclient-gui)
add_subdirectory (ddclient-gui)
