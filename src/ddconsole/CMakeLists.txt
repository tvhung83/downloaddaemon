cmake_minimum_required (VERSION 2.6)
project(ddconsole)

SET(ddconsole_SRCS ddconsole.cpp ../include/netpptk/netpptk.cpp ../include/crypt/md5.cpp)
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/include_ddconsole ../include)
SET_SOURCE_FILES_PROPERTIES(${ddconsole_SRCS} PROPERTIES COMPILE_FLAGS -DNO_BOOST_THREAD)
ADD_EXECUTABLE(ddconsole ${ddconsole_SRCS})
IF(WIN32)
	target_link_libraries(ddconsole ws2_32)
ENDIF(WIN32)

install(TARGETS ddconsole DESTINATION bin)
