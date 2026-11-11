CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wno-format-security
OBJ      = ngpc-dis.o vendor/dasm900.o

ngpc-dis: $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ)

%.o: %.cpp emu.h vendor/dasm900.h
	$(CXX) $(CXXFLAGS) -I. -Ivendor -c -o $@ $<

clean:
	rm -f ngpc-dis $(OBJ)
