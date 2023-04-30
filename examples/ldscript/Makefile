.PHONY: all clean

all: test

test: test_mod.o test.ld
	# gcc -Wl,-T,test1.ld test_mod.o -o test
	gcc test_mod.o -o test
	# bash link.sh

test_mod.o: test.o test.ld
	ld -r -T test.ld -o test_mod.o test.o

test.o: test.c

clean:
	rm -f *.o test
