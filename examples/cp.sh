#!/bin/bash
set -eux

SYSROOT=/root/phoenix-glibc/build/install
FILE=diff_vma

gcc \
  -g -o ${FILE}.out -v ${FILE}.c \
  -L "${SYSROOT}/lib" \
  -I "${SYSROOT}/include" \
  -I "/root/phx/userlib/lib/include" \
  -Wl,--rpath=${SYSROOT}/lib \
  -Wl,--dynamic-linker=${SYSROOT}/lib/ld-linux-x86-64.so.2 \
  -std=c11 \
  -lpthread \
  -lphx \
;
ldd -v ./${FILE}.out
./${FILE}.out
