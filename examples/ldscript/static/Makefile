.PHONY: all clean

CFLAGS = -g

SYSROOT=/root/glibc/rel-phx
SYSROOT=/root/glibc/rel-merge

all: test

test: test_mod.o test.ld
	# gcc -Wl,-T,test1.ld test_mod.o -o test
	gcc -g test_mod.o -o test \
		-L $(SYSROOT)/lib -L $(PWD) \
		-I $(SYSROOT)/include \
		-lphx \
		-Wl,--rpath=$(SYSROOT)/lib \
		-Wl,--dynamic-linker=$(SYSROOT)/lib/ld-linux-x86-64.so.2

test_mod.o: test.o test.ld
	ld -r -T test.ld -o test_mod.o test.o

test.o: test.c

clean:
	rm -f *.o *.so test
