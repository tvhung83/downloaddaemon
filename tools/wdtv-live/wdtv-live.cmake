# this one is important
SET(CMAKE_SYSTEM_NAME Linux)
#this one not so much
#SET(CMAKE_SYSTEM_VERSION 1)

# general VAR
SET(DD_PROJECT_PATH $ENV{DD_CHECKOUT_PATH})

# specify the cross compiler
SET(CMAKE_C_COMPILER $ENV{BINPATH}/mipsel-linux-gcc)
SET(CMAKE_CXX_COMPILER $ENV{BINPATH}/mipsel-linux-g++)

# where is the target environment 

SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Xlinker -rpath-link=${DD_PROJECT_PATH}/external/lib")

SET(CMAKE_FIND_ROOT_PATH 
		${SMP86XX_TOOLCHAIN_PATH}/mips-linux-gnu
		${DD_PROJECT_PATH}/external)

SET(CURL_INCLUDE_DIRs ${DD_PROJECT_PATH}/external/include)
SET(BOOST_INCLUDEDIRs ${DD_PROJECT_PATH}/external/include)

SET(DD_CONF_DIR "/apps/downloadd/etc/downloaddaemon")
SET(CMAKE_INSTALL_PREFIX /)

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
