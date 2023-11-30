#/bin/bash
set -eux

SYSROOT=/users/yzjing/phoenix-glibc/build/install
FILE=snap

g++ \
  -v -g -o ${FILE} -w ${FILE}.cpp \
  -I/users/yzjing/phoenix-userlib/lib/include \
  -L${SYSROOT}/lib \
  -I "${SYSROOT}/include" \
  -Wl,--rpath=${SYSROOT}/lib \
  -Wl,--dynamic-linker=${SYSROOT}/lib/ld-linux-x86-64.so.2 \
  -lphx 
  #-I"${SYSROOT}/include" \
  #-Wl,--dynamic-linker=${SYSROOT}/lib/libphx.so \ 
  #-std=c++11 \

ldd -v ./${FILE}
#./${FILE} 4
