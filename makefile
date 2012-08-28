cppc=g++-4.7 -Wall -std=c++11

e: main.cpp hexSet.o
	$(cppc) main.cpp hexSet.o -g `pkg-config --libs --cflags cairomm-1.0 jsoncpp` -o e

hexSet.o: hexSet
	objcopy --input-target binary --output-target elf32-i386 --binary-architecture i386 hexSet hexSet.o
hexSet: hexSetGen.cpp
	$(cppc) hexSetGen.cpp -o hexSetGen && ./hexSetGen hexSet