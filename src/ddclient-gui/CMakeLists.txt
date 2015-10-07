cmake_minimum_required (VERSION 2.6)
project(ddclient-gui)


SET(ddclient_gui_SRCS   ddclient_gui.cpp
						main.cpp
                        ddclient_gui_connect_dialog.cpp
                        ddclient_gui_add_dialog.cpp
                        ddclient_gui_update_thread.cpp
                        ddclient_gui_about_dialog.cpp
                        ddclient_gui_configure_dialog.cpp
                        ddclient_gui_status_bar.cpp
                        ddclient_gui_captcha_dialog.cpp
						ddclient_gui_list_handling.cpp
                        ../include/netpptk/netpptk.cpp
                        ../include/crypt/md5.cpp
                        ../include/language/language.cpp
                        ../include/downloadc/downloadc.cpp
                        ../include/downloadc/client_exception.cpp
						../include/cfgfile/cfgfile.cpp
						# headers without cpp files to include them into the project
						ddclient_gui_main_window.h)


SET(moc_headers			ddclient_gui.h
                        ddclient_gui_connect_dialog.h
                        ddclient_gui_add_dialog.h
                        ddclient_gui_configure_dialog.h
                        ddclient_gui_captcha_dialog.h)

SET(libs                "")


IF(WIN32)
        SET(GUI_TYPE WIN32)
        IF(MINGW)
                ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ddgui.o

                                        COMMAND windres.exe -I${CMAKE_CURRENT_SOURCE_DIR}

                                        -i${CMAKE_CURRENT_SOURCE_DIR}/ddgui.rc

                                        -o ${CMAKE_CURRENT_BINARY_DIR}/ddgui.o)

                SET(ddclient_gui_SRCS ${ddclient_gui_SRCS} ${CMAKE_CURRENT_BINARY_DIR}/ddgui.o)
        ELSE(MINGW)
                SET(ddclient_gui_SRCS ${ddclient_gui_SRCS} ddgui.rc)
        ENDIF(MINGW)
        SET(libs ${libs} ws2_32)

ENDIF(WIN32)
IF (APPLE)
#        SET(GUI_TYPE MACOSX_BUNDLE)
ENDIF (APPLE)



INCLUDE(CheckIncludeFiles)
INCLUDE(CheckIncludeFileCXX)
INCLUDE(CheckFunctionExists)
INCLUDE(CheckTypeSize)
INCLUDE(CheckCXXCompilerFlag)
INCLUDE(CheckCXXSourceCompiles)
INCLUDE(${CMAKE_CURRENT_SOURCE_DIR}/../CheckCxx11.cmake)


SET(DDCLIENT_GUI true)

IF(NOT VERSION)
        FIND_PACKAGE(Subversion)
        IF(Subversion_FOUND)
                Subversion_WC_INFO(${PROJECT_SOURCE_DIR} svn)
                SET(VERSION "r${svn_WC_REVISION}")
        ELSE(Subversion_FOUND)
                SET(VERSION "unknown")
        ENDIF(Subversion_FOUND)
ENDIF(NOT VERSION)
SET(DDCLIENT_GUI_VERSION "ON")

CONFIGURE_FILE(config.h.cmake ${CMAKE_BINARY_DIR}/include_ddclient-gui/config.h)
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/include_ddclient-gui/ ../include)


find_package(Qt4 COMPONENTS QtCore QtGui REQUIRED)


include(${QT_USE_FILE})
QT4_WRAP_CPP(ddclient_gui_MOC_SRCS ${moc_headers})
set(libs ${libs} ${QT_LIBRARIES})
IF(NOT USE_STD_THREAD)
        find_package(Boost 1.36.0 REQUIRED)
        INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR} ../include)
        ADD_EXECUTABLE(ddclient-gui ${GUI_TYPE} ${ddclient_gui_MOC_SRCS} ${ddclient_gui_SRCS})
        TARGET_LINK_LIBRARIES(ddclient-gui ${libs})
ELSE(NOT USE_STD_THREAD)
        ADD_EXECUTABLE(ddclient-gui ${GUI_TYPE} ${ddclient_gui_MOC_SRCS} ${ddclient_gui_SRCS})
        SET_TARGET_PROPERTIES(ddclient-gui PROPERTIES COMPILE_FLAGS ${CXX11_FLAG})
        TARGET_LINK_LIBRARIES(ddclient-gui ${libs})
ENDIF(NOT USE_STD_THREAD)


install(TARGETS ddclient-gui DESTINATION bin)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-gui/img/logoDD.png DESTINATION share/ddclient-gui/img/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-gui/img/1_connect.png DESTINATION share/ddclient-gui/img/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-gui/img/2_add.png DESTINATION share/ddclient-gui/img/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-gui/img/3_delete.png DESTINATION share/ddclient-gui/img/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-gui/img/4_stop.png DESTINATION share/ddclient-gui/img/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-gui/img/5_start.png DESTINATION share/ddclient-gui/img/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-gui/img/6_up.png DESTINATION share/ddclient-gui/img/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-gui/img/7_down.png DESTINATION share/ddclient-gui/img/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-gui/img/8_configure.png DESTINATION share/ddclient-gui/img/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-gui/img/9_activate.png DESTINATION share/ddclient-gui/img/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-gui/img/9_deactivate.png DESTINATION share/ddclient-gui/img/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-gui/img/10_delete_finished.png DESTINATION share/ddclient-gui/img/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-gui/img/11_copy_url.png DESTINATION share/ddclient-gui/img/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-gui/img/12_quit.png DESTINATION share/ddclient-gui/img/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-gui/img/13_about.png DESTINATION share/ddclient-gui/img/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-gui/img/14_select_all.png DESTINATION share/ddclient-gui/img/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-gui/img/15_delete_file.png DESTINATION share/ddclient-gui/img/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-gui/img/16_container.png DESTINATION share/ddclient-gui/img/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-gui/img/bullet_black.png DESTINATION share/ddclient-gui/img/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-gui/img/bullet_green.png DESTINATION share/ddclient-gui/img/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-gui/img/bullet_red.png DESTINATION share/ddclient-gui/img/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-gui/img/bullet_star.png DESTINATION share/ddclient-gui/img/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-gui/img/bullet_white.png DESTINATION share/ddclient-gui/img/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-gui/img/bullet_yellow.png DESTINATION share/ddclient-gui/img/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-gui/img/package.png DESTINATION share/ddclient-gui/img/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-gui/img/download_package.png DESTINATION share/ddclient-gui/img/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-gui/img/key.png DESTINATION share/ddclient-gui/img/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-gui/img/project-support.png DESTINATION share/ddclient-gui/img/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-gui/img/coins.png DESTINATION share/ddclient-gui/img/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-gui/img/bullet_captcha.png DESTINATION share/ddclient-gui/img/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-gui/img/go_top.png DESTINATION share/ddclient-gui/img/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-gui/img/go_bottom.png DESTINATION share/ddclient-gui/img/)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/pixmaps/ddclient-gui.png DESTINATION share/pixmaps)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/pixmaps/ddclient-gui.xpm DESTINATION share/pixmaps)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/applications/ddclient-gui.desktop DESTINATION share/applications/)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-gui/lang/Deutsch DESTINATION share/ddclient-gui/lang/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-gui/lang/Espanol DESTINATION share/ddclient-gui/lang/)

