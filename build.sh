export PATH=$PATH:/usr/local/gcc-linaro-arm-linux-gnueabihf/bin
cd ~/curl-7.44.0
CC=arm-linux-gnueabihf-gcc ./configure --host=arm --build=i686-linux --prefix=/home/robert/downloaddaemon/src/external
make
sudo make install
export CPLUS_INCLUDE_PATH="$CPLUS_INCLUDE_PATH;/usr/include/python2.7/"
./b2 --with-thread toolset=gcc-arm variant=release link=static 
./b2 --with-thread toolset=gcc-arm install variant=release link=static --prefix=/home/robert/downloaddaemon/src/external
export DSM52_TOOLCHAIN_PATH="/usr/local/gcc-linaro-4.9-2015.05-x86_64_arm-linux-gnueabihf"
export BINPATH="$DSM52_TOOLCHAIN_PATH/bin"
export DD_CHECKOUT_PATH=/home/robert/downloaddaemon
export CC=$BINPATH/arm-linux-gnueabihf-gcc
export CXX=$BINPATH/arm-linux-gnueabihf-g++
export LD=$BINPATH/arm-linux-gnueabihf-ld
export RANLIB=$BINPATH/arm-linux-gnueabihf-ranlib
export COMPILKIND='glibc hardfloat'
mkdir build
cd build
cmake -DCMAKE_CXX_FLAGS="-g -Wall" ../src/daemon -DCMAKE_TOOLCHAIN_FILE=../ls-410d.cmake