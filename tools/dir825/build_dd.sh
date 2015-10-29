mips-openwrt-linux-g++ -std=gnu++11 -o test.o test.cpp && scp -P 2345 test.o root@media.no-ip.org:/mnt/sda_part1/sys

CC=mips-openwrt-linux-gcc ./configure --host=mips-linux-musl --prefix=/home/robert/downloaddaemon/src/external

export ROOTDIR="${PWD}"
export CPPFLAGS="-I${ROOTDIR}/openssl/include -I${ROOTDIR}/zlib/include"
export LDFLAGS="-L${ROOTDIR}/openssl/libs -L${ROOTDIR}/zlib/libs"
export CROSS_COMPILE="mips-openwrt-linux"
export AR=${CROSS_COMPILE}-ar
export AS=${CROSS_COMPILE}-as
export LD=${CROSS_COMPILE}-ld
export CC=${CROSS_COMPILE}-gcc
export CXX=${CROSS_COMPILE}-g++
export NM=${CROSS_COMPILE}-nm
export RANLIB=${CROSS_COMPILE}-ranlib
export LIBS="-lssl -lcrypto"
CC=mips-openwrt-linux-gcc ./configure --with-ssl --with-zlib --enable-threaded-resolver --target=${CROSS_COMPILE} --host=${CROSS_COMPILE} --prefix=/media/psf/Home/playground/downloaddaemon/src/external

-- user-config.jam
using gcc : mips : mips-openwrt-linux-g++ ;
-- end of user-config.jam

./b2 --with-thread variant=release link=static 
./b2 --with-thread install variant=release link=static --prefix=/home/robert/downloaddaemon/src/external
export TOOLCHAIN_PATH="/home/robert/openwrt/staging_dir/toolchain-mips_34kc_gcc-4.8-linaro_musl-1.1.11"
export BINPATH="$TOOLCHAIN_PATH/bin"
export DD_CHECKOUT_PATH=/home/robert/downloaddaemon
export CC=$BINPATH/mips-openwrt-linux-gcc
export CXX=$BINPATH/mips-openwrt-linux-g++
export LD=$BINPATH/mips-openwrt-linux-ld
export RANLIB=$BINPATH/mips-openwrt-linux-musl-ranlib
export COMPILKIND='glibc hardfloat'
modify src/daemon/config.h.cmake (delete #cmakedefine BACKTRACE_ON_CRASH)

./DownloadDaemon -d --confdir /mnt/sda_part1/sys/etc/downloaddaemon -u root -g root -p /mnt/sda_part1/sys/tmp/downloadd.pid && tail -f /mnt/
sda_part1/sys/var/log/dd.log