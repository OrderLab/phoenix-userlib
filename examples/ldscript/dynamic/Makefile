.PHONY: all clean

SYSROOT=/root/glibc/rel-phx

all: test

test: test_mod.o test.ld
	# gcc -Wl,-T,test1.ld test_mod.o -o test
	gcc -g test_mod.o -o test \
		-L $(SYSROOT)/lib -L $(PWD) \
		-I $(SYSROOT)/include \
		-lphx libdata.so `#-ldata` \
		-Wl,--rpath=$(SYSROOT)/lib \
		-Wl,--dynamic-linker=$(SYSROOT)/lib/ld-linux-x86-64.so.2

test_mod.o: test.o test.ld libdata.so
	ld -r -T test.ld -o test_mod.o test.o

libdata.so: data_mod.o
	gcc -fPIC -shared data_mod.o -o libdata.so

data_mod.o: data.o test.ld
	ld -r -T test.ld -o data_mod.o data.o

test.o: test.c data.h

data.o: data.c data.h
	gcc -fPIC -shared -c data.c -o data.o

clean:
	rm -f *.o *.so test
