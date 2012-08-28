#include <cairomm/cairomm.h>
#include <algorithm>
#include <functional>
#include <random>
#include <cassert>
#include <iostream>
#include <fstream>
#include <json/json.h>
#include <chrono>

using namespace Cairo;
using namespace std;


extern char _binary_hexSet_start;
extern char _binary_hexSet_end;

typedef uint16_t glyph;
glyph* hexSet = (glyph*)&_binary_hexSet_start;
unsigned const hexSetSize = ((&_binary_hexSet_end - &_binary_hexSet_start)*sizeof(char))/sizeof(glyph);


unsigned long parseHex(char *s){ //source: http://www.arrizza.com/snippets/cpp/hatol.html
	if (*s && *s == '0' && *(s + 1) && (*(s+1) == 'x' || *(s+1) == 'X'))
		s += 2;

	unsigned long d;
	unsigned l;
	for(l = 0; *s ; ++s){
		if (*s >= '0' && *s <= '9')
			d = *s - '0';
		else if (*s >= 'a' && *s <= 'f')
			d = *s - 'a' + 10;
		else if (*s >= 'A' && *s <= 'F')
			d = *s - 'A' + 10;
		else
			break;
		l = (l << 4) + d;
	}

	return l;
}

template<typename T>
T mongle(T previous){
	const T arbBytes = 0xFABB705E;
	return (22695477*(arbBytes+previous));
}

template <typename T>
vector<T> chooseRand(unsigned many, unsigned from, uint32_t seed){
	vector<T> out;
	assert(many <= from);
	for(unsigned i=0; i<many; ++i){
		T newun = seed%(from-i);
		unsigned lessers=0;
		for(T lesser : out) if(lesser <= newun) ++lessers;
		out.push_back(newun+lessers);
		seed = mongle(seed);
	}
	return out;
}


uint16_t hexGlyphFromRand(uint32_t seed){ //deprecated; doesn't deal with transpositions, see hexGlyphFromPregenerated;
	unsigned nholes = 4+(seed%3);
	seed = mongle(seed);
	auto choices = chooseRand<uint8_t>(nholes, 12, seed);
	uint16_t bitmarks = -1;
	for(uint8_t choice : choices)
		bitmarks &= ~(1<<choice);
	return bitmarks;
}

uint16_t hexGlyphFromPregenerated(uint32_t index){
	return hexSet[index%hexSetSize];
}

void drawHexInvader(double overlap,
	                  double centerx, double centery, double span,
	                  Cairo::RefPtr<Cairo::Context>& cr,
	                  uint32_t seed){
	double hexDiameterOverlap = (span-2*overlap)/4+2*overlap; //4 diameters in total cross the span;
	double hexDiameter        = (span)/4;
	double hexRunOverlap  = hexDiameterOverlap/2 * cos(M_PI/6);
	double hexRiseOverlap = hexDiameterOverlap/2 * sin(M_PI/6);  //these describe the top left edge of the hex; [although ... they're transposed in practice. a stupidum.]
	double distanceHypotenuse = hexDiameter * cos(M_PI/6);
	double hexRun  = distanceHypotenuse*cos(M_PI/6);
	double hexRise = distanceHypotenuse*sin(M_PI/6);  //these describe the gaps between hexen;
	uint32_t bitmarks = hexGlyphFromPregenerated(seed);
	auto isDrawing = [bitmarks](unsigned n)->bool{return (bool)(bitmarks & (1<<n));};
	auto drawHex =  [=, &cr](double x, double y){ //centered on x, y;
		cr->move_to(x-hexRiseOverlap, y-hexRunOverlap);
		cr->rel_line_to(hexDiameterOverlap/2, 0);
		cr->rel_line_to(hexRiseOverlap, hexRunOverlap);
		cr->rel_line_to(-hexRiseOverlap, hexRunOverlap);
		cr->rel_line_to(-hexDiameterOverlap/2, 0);
		cr->rel_line_to(-hexRiseOverlap, -hexRunOverlap);
		cr->close_path();
	};
	if(isDrawing(0)){
		drawHex(centerx-2*hexRun, centery-2*hexRise);
		drawHex(centerx+2*hexRun, centery-2*hexRise);
	}
	if(isDrawing(1)){
		drawHex(centerx-2*hexRun, centery);
		drawHex(centerx+2*hexRun, centery);
	}
	if(isDrawing(2)){
		drawHex(centerx-2*hexRun, centery+2*hexRise);
		drawHex(centerx+2*hexRun, centery+2*hexRise);
	}
	if(isDrawing(3)){
		drawHex(centerx-hexRun, centery-3*hexRise);
		drawHex(centerx+hexRun, centery-3*hexRise);
	}
	if(isDrawing(4)){
		drawHex(centerx-hexRun, centery-hexRise);
		drawHex(centerx+hexRun, centery-hexRise);
	}
	if(isDrawing(5)){
		drawHex(centerx-hexRun, centery+hexRise);
		drawHex(centerx+hexRun, centery+hexRise);
	}
	if(isDrawing(6)){
		drawHex(centerx-hexRun, centery+3*hexRise);
		drawHex(centerx+hexRun, centery+3*hexRise);
	}
	if(isDrawing(7)){
		drawHex(centerx, centery-4*hexRise);
	}
	if(isDrawing(8)){
		drawHex(centerx, centery-2*hexRise);
	}
	if(isDrawing(9)){
		drawHex(centerx, centery);
	}
	if(isDrawing(10)){
		drawHex(centerx, centery+2*hexRise);
	}
	if(isDrawing(11)){
		drawHex(centerx, centery+4*hexRise);
	}
}
void drawHexInvader(Color col,   double overlap,
	                  Cairo::RefPtr<Cairo::ImageSurface>& image,
	                  uint32_t seed){
	double w = image->get_width();
	double h = image->get_height();
	double centerx = w/2;
	double centery = h/2;
	double span = ((double)5*sin(M_PI/3)/4 *w <= h)?
		w:
		h/((double)5*sin(M_PI/3)/4);
	auto cr = Context::create(image);
	cr->set_source_rgb(col.r,col.g,col.b);
	drawHexInvader(overlap,   centerx, centery, span,   cr, seed);
	cr->fill();
}
template <typename T, typename Seed>
vector<T> partitionGen(unsigned nparts, unsigned bandw, Seed seed){ //generates a partitioning with an even probability distribution. Much quicker than determining the partitioning from recursive applications of partitionn to derive a partitioning from a natural encoding of a partition;
	vector<T> out(nparts);
	srand(seed);
	unsigned genRange = bandw;
	for(auto i=begin(out); i<end(out); ++i){
		unsigned gen = rand()%genRange;
		*i = (gen<bandw)?
			gen:
			*(i-(gen-bandw+1));
		++genRange;
	}
	sort(begin(out), end(out), less<T>());
	return out;
}

struct Color{
	struct Component{
		uint8_t v;
		Component(uint8_t in):v(in){}
		Component(double in):v((uint8_t)(in*255)){}
		operator double(){return (double)v/255;} //I just realized I find these to be rather dangerous.
		operator uint8_t(){return v;}
	};
	Component r,g,b,a;
	Color(double re, double gr, double bl):Color(re,gr,bl,1){}
	Color(double re, double gr, double bl, double al):r(re),g(gr),b(bl),a(al){}
};

Color colorFromHue(double in){ //in is between 0 and 1, zero meaning red, and 1 meaning red ;)
	switch((unsigned)(in*6)){
	case 0: //red is high, green is increasing;
		return Color(255, (uint8_t)(255*in*6), 0);
	case 1: //green is high, red is decreasing;
		return Color((uint8_t)(255*(1 - (in*6-1))), 255, 0);
	case 2: //green is high, blue is increasing;
		return Color(0, 255, (uint8_t)(255*(in*6-2)));
	case 3: //.. you get the gist of it;
		return Color(0, (uint8_t)(255*(1- (in*6-3))), 255);
	case 4:
		return Color((uint8_t)(255*(in*6-4)), 0, 255);
	default: //default cause there's a chance it could be 6;
		return Color(255, 0, (uint8_t)(255*(1- (in*6-5))));
	}
}

int main(int argc, char** argv){
	string outfile = "out.png";
	uint32_t startingSeed = (uint32_t)chrono::high_resolution_clock::now().time_since_epoch().count();
	Color faceColor(0,0,0);
	Color backColor(1,1,1);
	Color plaqueColor(0.9,0.9,0.9);
	unsigned nw = 10;
	unsigned nh = 5;
	bool isHexy = false;
	bool isRandom = true; //if this is false, the system methodically counts through each glyph;
	bool allowRotationals = false;
	unsigned faceDiam = 32;
	unsigned plaqueDiam = 40;
	unsigned faceSeparation = 50;
	unsigned colorN =0;
	vector<Color> colors;
	double overlap = 0;
	
	if(argc > 1){
		using namespace Json;
		ifstream orderf(argv[1]);
		if(!orderf.is_open()){
			cerr<<"file; \""<<argv[1]<<"\" is not existing."<<endl;
			return 1;
		}
		Value order;
		Reader reader;
		if(!reader.parse(orderf,order)){
			cerr<<"bad order file";
			return 1;
		}

		startingSeed = order.get("seed", startingSeed).asUInt();
		isRandom = !order.get("iterative", !isRandom).asBool();
		nw = order.get("number of columns", nw).asUInt();
		nh = order.get("number of rows", nh).asUInt();
		isHexy = order.get("hexagonally packed", isHexy).asBool();
		allowRotationals = order.get("rotationally symetric faces", allowRotationals).asBool();
		faceDiam = order.get("face diameter", faceDiam).asUInt();
		faceSeparation = order.get("distance between faces", faceSeparation).asDouble();
		overlap = order.get("hexagon protrusion", overlap).asDouble();
		colorN = order.get("number of colors", colorN).asUInt(); //currently innert;
		plaqueDiam = order.get("face border diameter", plaqueDiam).asUInt();
		outfile = order.get("output file name", outfile).asString();
		auto tryColor = [&order](string const& name, Color& v){
			auto possibleColor = order[name];
			if(!!possibleColor){ //"!!" as doesn't have a operator(bool);
				v = Color(possibleColor["r"].asDouble(),
				          possibleColor["g"].asDouble(),
				          possibleColor["b"].asDouble());
			}
		};
		tryColor("face color", faceColor);
		tryColor("face plaque color", plaqueColor);
		tryColor("background color", backColor);

		assert(nw && nh);
	}

	if(colorN && colors.empty()){
		double division = (double)1/colorN;
		curhue = 0;
		for(unsigned i=0; i<colorN; ++i){
			colors.push_back(colorFromHue(curhue));
			curhue+=division;
		}
	}

	uint32_t seed = mongle(startingSeed);
	RefPtr<ImageSurface> surface;
	RefPtr<Cairo::Context> cr;
	auto drawRow = [=, &plaqueColor, &faceColor, &cr, &seed](double x, double y){
		cr->set_source_rgb(plaqueColor.r, plaqueColor.g, plaqueColor.b);
		for(unsigned i=0;   i < nw;   ++i){
			double xt = x+faceSeparation*i;
			cr->arc(xt, y, plaqueDiam/2, 0, 2*M_PI);
		}
		cr->fill();
		cr->set_source_rgb(faceColor.r, faceColor.g, faceColor.b);
		uint32_t myseed = isRandom?mongle(seed+3):seed;
		for(unsigned i=0;   i < nw;   ++i, myseed = isRandom ? mongle(myseed+3) : myseed+1){
			double xt = x+faceSeparation*i;
			drawHexInvader(overlap,   xt, y, faceDiam,   cr, myseed);
		}
		cr->fill();
	};
	if(!isHexy || nh == 1){
		surface = ImageSurface::create(FORMAT_ARGB32, faceSeparation*(nw+1), faceSeparation*(nh+1));
		cr = Cairo::Context::create(surface);
		cr->set_source_rgb(backColor.r, backColor.g, backColor.b);
		cr->paint();
		double y = faceSeparation;
		for(unsigned i=0; i < nh; ++i,y+=faceSeparation,seed = isRandom ? mongle(seed) : seed+nw){
			drawRow(faceSeparation,y);
		}
	}else{
		double run  = faceSeparation*sin(M_PI/6);
		double rise = faceSeparation*cos(M_PI/6);
		surface = ImageSurface::create(FORMAT_ARGB32, faceSeparation*(nw+1)+run, rise*(nh-1)+2*faceSeparation);
		cr = Cairo::Context::create(surface);
		cr->set_source_rgb(backColor.r, backColor.g, backColor.b);
		cr->paint();
		double x = faceSeparation;
		double y = faceSeparation;
		bool oscilator=true;
		unsigned i=0;
		while(i<nh){
			drawRow(x,y);
			seed = isRandom? mongle(seed) : seed+nw;
			++i;
			if(oscilator){
				oscilator = false;
				x += run;
			}else{
				oscilator = true;
				x -= run;
			}
			y += rise;
		}
	}

	surface->write_to_png(outfile);
}

void drawConcentrics(Cairo::RefPtr<Cairo::ImageSurface>& surface, uint32_t seed){
	auto cr = Context::create(surface);
	cr->set_source_rgb(1,1,1);
	static unsigned const minimumRingBreadth = 3;
	static unsigned const maxNRings = 4;
	unsigned span = surface->get_width();
	unsigned minInnerDilation = (unsigned)(span*0.47);
	assert(span >= minInnerDilation+minimumRingBreadth*maxNRings);

	unsigned nrings;
	float mainangle;
	vector<unsigned> ringDilations;
	vector<unsigned> ringBreadths;
	vector<vector<float>> ringPartitionings;
	//nrings:
	switch(mongle<uint32_t>(seed+2)&3){
	case 0 :
		nrings = 2; break;
	case 1 : case 2:
		nrings = 3; break;
	case 3 :
		nrings = maxNRings;
	}
	//mainangle:
	unsigned const angleRandMax = 1<<6;
	unsigned angleRand = mongle(seed+4)&(angleRandMax-1);
	mainangle = (M_PI*angleRand)/angleRandMax;
	//ringDilations:
	ringDilations = partitionGen<unsigned, uint32_t>(nrings+1, (span-minInnerDilation)/2, mongle(seed));
	for(unsigned i=0; i < nrings; ++i){
		ringDilations[i] += (i+1)*minimumRingBreadth;
	}
	//ringBreadths:
	for(unsigned i=0; i<nrings; ++i){

	}
}