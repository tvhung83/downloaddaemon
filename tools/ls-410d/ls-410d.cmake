# this one is important
SET(CMAKE_SYSTEM_NAME Linux)
#this one not so much
#SET(CMAKE_SYSTEM_VERSION 1)

# general VAR
SET(DD_PROJECT_PATH /home/robert/downloaddaemon)

# specify the cross compiler
SET(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
SET(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)

# where is the target environment 

SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Xlinker -rpath-link=${DD_PROJECT_PATH}/src/external/lib")

SET(CMAKE_FIND_ROOT_PATH 
		/usr/local/gcc-linaro-arm-linux-gnueabihf/arm-linux-gnueabihf
		${DD_PROJECT_PATH}/src/external)

SET(CURL_INCLUDE_DIRs ${DD_PROJECT_PATH}/src/external/include)
SET(BOOST_INCLUDEDIRs ${DD_PROJECT_PATH}/src/external/include)

SET(DD_CONF_DIR "/etc/downloaddaemon")
SET(CMAKE_INSTALL_PREFIX /)

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
