#include <fstream>
#include <iostream>
#include <functional>

using namespace std;

typedef uint16_t glyph;
bool checkFirstLine(uint16_t v){
	return (bool)(v&((1<<0)|(1<<3)|(1<<7)));
}

template<class uint>
unsigned numberOfSetBits(uint n) {
  unsigned int c; // c accumulates the total bits set in v
  for (c = 0; n; c++) 
    n &= n - 1; // clear the least significant bit set
  return c;
}

int main(int argc, char** argv){
	if(argc < 2){
		cerr<<"nope, needs an outfile name"<<endl;
		return 1;
	}
	ofstream of(argv[1], ios_base::binary|ios_base::out|ios_base::trunc);
	glyph cur = 0;
	glyph upper = (1<<12);
	do{
		if(checkFirstLine(cur)){ //then cur has a bit set in the first line[something is on]; we must allow no empty first line. This is how we ensure that we don't get two glyphs that are translations of each other;
			if(numberOfSetBits(cur) >= 12-6){ //then cur has enough bits switched on to be considered;
				of.write(reinterpret_cast<char*>(&cur), sizeof(glyph));
			}
		}
		++cur;
	}while(cur != upper);
}