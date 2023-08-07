# Phoenix-glibc compilation

**1. Update the version of Debian **

Before making the image, vim `/scripts/mkimg.sh` in orbit directory and replace **buster** with **bookworm**. 

**2. Updating the guest_setup.sh script**

Then, in `/scripts/guest_setup`, comment out the packages from `llvm-6.0` to `lld-6.0`. Other packages remain the same. An error may occur during running the script, it can be solved by running `apt autoremove ca-certificates-java`.(Details about the error may be updated later)

**3. Compile phoenix-glibc**

First, git clone the respository of phoenix-glibc and checkout to branch `draft`. To avoid unnecessary system error(which may crash the whole vm), we choose to install phoenix-glibc locally. We can create a `build` directory in `phoenix-glibc`, then create another directory `install` after entering `build`. 

In `phoenix-glibc/build`, we first run `../configure --prefix=~/phoenix-glibc/build/install` to clarify the install route. Then, run `make && make install`. 

**4. Use phoenix-glibc**

During compilation, some flags should be added like below(linking with `libphx.so` is omitted here):

``````bash
#!/bin/bash

# An example for compiling with phoenix-glibc
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
``````