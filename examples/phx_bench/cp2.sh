#/bin/bash
set -eux

SYSROOT=/root/phoenix-glibc/build/install
FILE=m_test

g++ \
  -v -g -o ${FILE} -w ${FILE}.cpp \
  -I/root/phx/userlib/lib/include \
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
