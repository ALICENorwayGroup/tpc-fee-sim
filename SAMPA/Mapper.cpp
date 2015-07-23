#include "Mapper.h"
#include <cmath>
#include "GlobalConstants.h"
Mapper::Mapper(){

}

//Uses hw address in black event files to return sampa channel.
int Mapper::getSampaChannel(uint16_t hw){
	std::ifstream iff("altro-sampa-mapping.data");
	std::string line;

	while(!iff.eof()){
		std::getline(iff, line);
		std::cout << "Line: " << line;
		int currentHw = std::stoi(line.substr(0, line.find(" ")));

		if(currentHw == hw){
			return std::stoi(line.substr(line.find(" ")));
		}
	}
	iff.close();
	return -1;
}

unsigned char Mapper::getBranch(uint16_t data){
	return (data >> 11);
}

unsigned char Mapper::getFec(uint16_t data){
	return (data >> 7) & 0xf;
}

unsigned char Mapper::getAltro(uint16_t data){
	return (data >> 4) & 7;
}

unsigned char Mapper::getChannel(uint16_t data){
	return data & 0xf;
}
