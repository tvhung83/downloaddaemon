cmake_minimum_required (VERSION 2.6)
set(CMAKE_VERBOSE_MAKEFILE true)
set(CMAKE_BUILD_TYPE Debug)

project (DownloadDaemon)
add_subdirectory (src/daemon)
#add_subdirectory (src/lib)

project (ddconsole)
add_subdirectory (src/ddconsole)
#add_subdirectory (src/lib)

project (ddclient-gui)
add_subdirectory (src/ddclient-gui)
#add_subdirectory (src/lib)



