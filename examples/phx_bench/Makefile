SYSROOT=/root/phoenix-glibc/build/install
LIBS= -I ${SYSROOT}/include -I /root/phx/userlib/lib/include -L${SYSROOT}/lib -Wl,--rpath=${SYSROOT}/lib -Wl,--dynamic-linker=${SYSROOT}/lib/ld-linux-x86-64.so.2 
CXXFLAGS = -v -lphx -O3
TARGET=snap
SRC=snap.cpp

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(LIBS) -o $@ $<

.PHONY: clean
clean:
	rm -f $(TARGET)
