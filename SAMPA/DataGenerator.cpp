#include "DataGenerator.h"
#include "Huffman.h"
/*
* Generating samples
* */
void DataGenerator::t_sink(void){
  if(constants::DG_SIMULTION_TYPE == 1){
    standardSink();
  } else if(constants::DG_SIMULTION_TYPE == 2){
    incrementingOccupancySink();
  } else if(constants::DG_SIMULTION_TYPE == 3){
    globalRandomnessSink();
  } else if(constants::DG_SIMULTION_TYPE == 4){
    sendBlackEvents();
  } else {
    sendGaussianDistribution();
  }
}

//OLD! -----------------------------
void DataGenerator::createFlux(std::vector<int> &values){
  RandomGenerator generator;
  for (int i = 0; i < values.size(); ++i)
  {
    if(generator.generate(0, 100) <= 10 && values[i] - 20 > 0){
      values[i] -= 20;
      if(i - 100 > 0 && values[i] + 20 <= 100){
        values[i - 100] += 20;
      } else if(i + 100 < values.size() - 1 && values[i] + 20 <= 100){
        values[i + 100] += 20;
      }
    }

    if(generator.generate(0, 1000) <= 1){
      values[i] = 100;
      values[(i - 1) < 0 ? i - 1 : i + 1] = 0;
    }
  }
}

//OLD
int DataGenerator::createSingleFlux(){
  int value = constants::DG_OCCUPANCY;
  RandomGenerator generator;
  if(generator.generate(0, 100) <= 10){
    if(generator.generate(0, 100) <= 10){
      value -= 15;
    } else if(generator.generate(0, 100) > 50 && generator.generate(0, 100) <= 70){
      value += 15;
    }

  } else if(generator.generate(0, 1000) <= 1){
    value = 100;
  } else {
    value = generator.generate(value - 5, value + 5);
  }
  return value;
}

//OLD
void DataGenerator::globalRandomnessSink(){
  int64_t packetCounter = 1;
  int currentSample = 0;
  int currentTimeWindow = 1;
  int lastData = 0;
  std::vector<int> values (constants::NUMBER_OF_SAMPA_CHIPS * constants::SAMPA_NUMBER_INPUT_PORTS, 1);

  RandomGenerator randomGenerator;
  //std::cout << "Reading black events..";
  //readBlackEvents();

  //int rounds = ((constants::NUMBER_OF_SAMPA_CHIPS * constants::SAMPA_NUMBER_INPUT_PORTS) - 1) * 30;

  initOccupancy();
  while(currentTimeWindow <= constants::NUMBER_TIME_WINDOWS_TO_SIMULATE){

    std::cout << "Sending data " << currentSample << " | data value: " << lastData <<  endl;
    for(int i = 0; i < constants::NUMBER_OF_SAMPA_CHIPS * constants::SAMPA_NUMBER_INPUT_PORTS; i++)
    {
      //int flux = createSingleFlux();
      lastData = generateCore(i, currentTimeWindow, packetCounter, occupancyPoints[currentTimeWindow - 1]);
      //lastData = generateCore(i, currentTimeWindow, packetCounter, createSingleFlux());
      if(lastData != 0){
        packetCounter++;
      }
    }

    currentSample++;
    //Increments timeWindow
    if(currentSample == constants::NUMBER_OF_SAMPLES_IN_EACH_TIME_WINDOW )//1021 samples
    {
      currentTimeWindow++;
      std::cout << "Current packetCounter: " << packetCounter << " window: " << currentTimeWindow << endl;
      currentSample = 0;
    }
    wait((constants::DG_WAIT_TIME), SC_NS);
  }
}
//Standard implementation with fixed Occupancy. OLD!
void DataGenerator::standardSink(){
  int64_t packetCounter = 1;
  int currentSample = 0;
  int currentTimeWindow = 1;

  //While we still have timewindows to send
  while(currentTimeWindow <= constants::NUMBER_TIME_WINDOWS_TO_SIMULATE)
  {
    //Loop each channel
    for(int i = 0; i < constants::NUMBER_OF_SAMPA_CHIPS * constants::SAMPA_NUMBER_INPUT_PORTS; i++)
    {

      if(generateCore(i, currentTimeWindow, packetCounter, constants::DG_OCCUPANCY) != 0){
        packetCounter++;
      }
    }
    currentSample++;
    std::cout << "Sending data " << currentSample << endl;
    //Increments timeWindow
    if(currentSample == constants::NUMBER_OF_SAMPLES_IN_EACH_TIME_WINDOW )//1021 samples
    {
      currentTimeWindow++;
      std::cout << "Current packetCounter: " << packetCounter << " window: " << currentTimeWindow << endl;
      currentSample = 0;
    }
    wait((constants::DG_WAIT_TIME), SC_NS);
  }
}
//OLD!
void DataGenerator::initOccupancy(){
  occupancyPoints[0] = 30;
  occupancyPoints[1] = 30;
  occupancyPoints[2] = 30;
  occupancyPoints[3] = 30;
  occupancyPoints[4] = 90;
  occupancyPoints[5] = 90;
  occupancyPoints[6] = 100;
  occupancyPoints[7] = 100;
  occupancyPoints[8] = 100;
  occupancyPoints[9] = 100;
  occupancyPoints[10] = 100;
  occupancyPoints[11] = 90;
  occupancyPoints[12] = 90;
  occupancyPoints[13] = 80;
  occupancyPoints[14] = 80;
  occupancyPoints[15] = 60;
  occupancyPoints[16] = 60;
  occupancyPoints[17] = 30;
  occupancyPoints[18] = 30;
  occupancyPoints[19] = 30;
  occupancyPoints[20] = 30;
  occupancyPoints[21] = 30;
  occupancyPoints[22] = 30;
  occupancyPoints[23] = 0;
  occupancyPoints[24] = 0;

}

//Implementation with increasing occupancy. OLD!
void DataGenerator::incrementingOccupancySink(){
  int currentTimeWindow = 1;
  int64_t packetCounter = 1;
  int occupancy = 10;
  int currentSample = 0;
  RandomGenerator g;
  int newOccupancy = 10;
  int lastData = 0;
  while(currentTimeWindow <= constants::NUMBER_TIME_WINDOWS_TO_SIMULATE)
  {
    for(int i = 0; i < constants::NUMBER_OF_SAMPA_CHIPS * constants::SAMPA_NUMBER_INPUT_PORTS; i++)
    {

      newOccupancy = g.generate(occupancy - 5, occupancy + 5);
      lastData = generateCore(i, currentTimeWindow, packetCounter, newOccupancy);
      if(lastData != 0){
        packetCounter++;
      }


    }
    std::cout << "Sending data " << currentSample << " | data value: " << lastData <<  endl;

    currentSample++;
    if(currentSample == constants::NUMBER_OF_SAMPLES_IN_EACH_TIME_WINDOW )//1021 samples
    {
      occupancyPoints[currentTimeWindow - 1] = occupancy;
      if(currentTimeWindow % (constants::NUMBER_TIME_WINDOWS_TO_SIMULATE / constants::TIME_WINDOW_OCCUPANCY_SPLIT) == 0){

        std::cout << "Occupancy: " << occupancy << endl;
        std::cout << "Current packetCounter: " << packetCounter << " window: " << currentTimeWindow << endl;
        occupancy = occupancy + constants::TIME_WINDOW_OCCUPANCY_SPLIT;

      }
      currentTimeWindow++;
      currentSample = 0;
    }
    wait((constants::DG_WAIT_TIME), SC_NS);
  }
}

//OLD
int DataGenerator::generateCore(int portNumber, int currentTimeWindow, int packetCounter, int occupancy){

  RandomGenerator randomGenerator;

  if(randomGenerator.generate(0, 100) <= occupancy){

    Sample sample(currentTimeWindow, packetCounter, 1, randomGenerator.generate(20, 60));
    porter_DG_to_SAMPA[portNumber]->nb_write(sample);
    //write_log_to_file_sink(packetCounter, portNumber, currentTimeWindow);
    return sample.data;

  }
  Sample emptySample;
  porter_DG_to_SAMPA[portNumber]->nb_write(emptySample);
  return emptySample.data;
}
//OLD end --------------------------------------------------------


/*
* Reads in different data sets to a Datamap object.
* Iterates it and sends it to the different channels.
* Uses NUMBER_TIME_WINDOWS_TO_SIMULATE variable form GlobalConstants.h to decide how many timewindows to read in.
*/
void DataGenerator::sendBlackEvents(){

  std::cout << "Reading events into memory" << endl;
  Datamap dataMap = readBlackEvents(); //readBlackEvents();//readPileUpEvents();
  int counter = 0;
  std::cout << sc_time_stamp() << " Finished reading events into memory " << dataMap.size() << endl;
  for(int j = 0; j < constants::NUMBER_TIME_WINDOWS_TO_SIMULATE / dataMap.size(); j++){
  for(std::vector< DataEntry >::iterator it = dataMap.begin(); it != dataMap.end(); ++it){

    for(int i = 0; i < 1021; i++){
      for(std::map<int, std::list<Sample>>::iterator mit = it->begin(); mit != it->end(); ++mit){
        int channel = mit->first;
        //Reverse iterator through list
        porter_DG_to_SAMPA[channel]->nb_write(mit->second.back());
        mit->second.pop_back();
        counter++;
      }
      std::cout << "Progress: " << ((counter / (1021.0 * 32.0 * constants::NUMBER_TIME_WINDOWS_TO_SIMULATE)) * 100.0) << '\r';
      wait((constants::DG_WAIT_TIME), SC_NS);
    }


  }
}
}
/*
Filinfo:
ddl start pos = 4.
hw start pos = 3.
typedef std::map< int, std::list<Sample> > DataEntry;
typedef std::vector< DataEntry > Datamap;
Reads in black events, doesnt matter if it is pileup or not. just that the format is correct.
*/
DataGenerator::Datamap DataGenerator::readBlackEvents(){
  std::ifstream inputFile(constants::DATA_FILE);
  std::vector<uint16_t> words;
  std::string line;
  Mapper mapper;
  Huffman huffman;
  int samplePrefix, samplePostfix, hwAddr, nrOfSamples, startTime, sampaAddr;
  Datamap map;
  DataEntry entry;
  int sampleId = 0;
  int timeFrame = 1;
  int count = 0;

  if (!inputFile.good()) {
    std::cerr << "can not open file " << constants::DATA_FILE << " for reading of real event data" << std::endl;
    return map;
  }

  std::cout << "reading event raw data from file " << constants::DATA_FILE << std::endl;

  //Read everything.
  while(!inputFile.eof()){

    //Break when selected number of timeframes is done.
    if(timeFrame > constants::NUMBER_TIME_WINDOWS_TO_SIMULATE){
      break;
    }

    std::getline(inputFile, line);

    //Finding hardware addr.
    if(line.find("hw") == 0){
      std::list<Sample> list;
      hwAddr = std::stoi(line.substr(3));
      //Uncomment if you need the exact mapping! See Mapper.h/Mapper.cpp for more info.
      /*
      sampaAddr = mapper.getSampaChannel(hwAddr);
      if(sampaAddr < 0){
      continue;
    }*/

    //Gets values from the file and calculates number of empty packets at start/end.
    std::getline(inputFile, line);
    nrOfSamples = std::stoi(line.substr(line.find(" ")));
    startTime = std::stoi(line.substr(0, line.find(" ")));
    samplePrefix = constants::NUMBER_OF_SAMPLES_IN_EACH_TIME_WINDOW - startTime;
    samplePostfix = startTime - nrOfSamples;

    uint16_t prev = 0;
    //Empty samples in the front.
    for(int i = 0; i < samplePrefix; i++){
      Sample sample;
      sample.timeWindow = timeFrame;
      list.push_back(sample);

      //Huffman generation.
      words.push_back(constants::HUFFMAN_PREFIX);
      prev = 0;
    }

    //Real samples.
    for(int i = 0; i < nrOfSamples; i++){
      std::getline(inputFile, line);
      int signal = std::stoi(line.substr(line.find(" ")));
      Sample sample(timeFrame, sampleId, 1, signal);
      list.push_back(sample);
      sampleId++;

      //Huffman!
      uint16_t temp = signal;
      int16_t t_res = (temp - prev) + constants::HUFFMAN_PREFIX;
      uint16_t res = t_res;
      words.push_back(res);
      prev = temp;
    }

    //Empty samples after.
    for(int i = 0; i < samplePostfix; i++){
      Sample sample;
      sample.timeWindow = timeFrame;
      list.push_back(sample);

      //Huffman generation
      words.push_back(constants::HUFFMAN_PREFIX);
      prev = 0;
    }

    //Insert entire timeframe for 1 channel.
    entry.insert(std::pair<int, std::list<Sample>>(count, list));
    count++;

    //When number of channels is reached, start new timeframe.
    if(count == constants::SAMPA_NUMBER_INPUT_PORTS * constants::NUMBER_OF_SAMPA_CHIPS){
      map.push_back(entry);
      entry.clear();
      count = 0;
      timeFrame++;
    }

  }

}
//Generate huffman table and write it to file.
HuffCodeMap codes;
huffman.CreateTree(words, codes);
huffman.WriteCodesToFile(constants::HUFFMAN_TREE_FILE_NAME, codes);

return map;
}

DataGenerator::Datamap DataGenerator::readEvents(){
  std::ifstream inputFile(constants::DATA_FILE);
  std::vector<uint16_t> words;
  std::string line;
  Mapper mapper;
  Huffman huffman;
  Datamap map;
  DataEntry entry;
  int sampleId = 0;
  if (!inputFile.good()) {
    std::cerr << "can not open file " << constants::DATA_FILE << " for reading of real event data" << std::endl;
    return map;
  }

  int timeFrame = 1;
	int hwAddr;
	int i = 1021;
	int count = 0;
	while(!inputFile.eof()){
		std::getline(inputFile, line);
		if(timeFrame > constants::NUMBER_TIME_WINDOWS_TO_SIMULATE){
      break;
    }

		//Finding hardware addr.
		if(line.find("hw") == 0){
      std::list<Sample> list;
			hwAddr = std::stoi(line.substr(3));
			i = 1021;
			while(i > 0){

				std::getline(inputFile, line);
				if(line.find("hw") == 0){
					for(int j = 0; j < i; j++){
            Sample sample;
            sample.timeWindow = timeFrame;
            list.push_back(sample);
					}
					break;
				}
				int signal = std::stoi(line.substr(line.find(" ")));
				int time = std::stoi(line.substr(0, line.find(" ")));
        Sample sample(timeFrame, sampleId, 1, signal);
        list.push_back(sample);
        sampleId++;
				i--;
				int diff = (i - time);
				for(int j = 0; j < diff; j++){
          Sample sample;
          sample.timeWindow = timeFrame;
          list.push_back(sample);
					i--;
				}
			}
      //Insert entire timeframe for 1 channel.
      entry.insert(std::pair<int, std::list<Sample>>(count, list));
      count++;

      //When number of channels is reached, start new timeframe.
      if(count == constants::SAMPA_NUMBER_INPUT_PORTS * constants::NUMBER_OF_SAMPA_CHIPS){
        map.push_back(entry);
        entry.clear();
        count = 0;
        timeFrame++;
      }
		}


	}


return map;
}


/*
* FAKE pileup (Simulated pileup data)
* file format:
* #sector #pad #padrow #timebin #signal #qxtra(not used)
* Only 1000 timebins per event. (Adds 22 empty at end).
*/
DataGenerator::Datamap DataGenerator::readPileUpEvents(){
  std::ifstream inputFile(constants::DATA_FILE);
  int samplePostfix = 22;
  int sector, pad, padnr, time, value;
  double q;
  int sampleId = 0;
  int timeframe = 1;
  int count = 0;
  std::string line;
  Datamap map;
  DataEntry entry;
  Huffman huffman;
  std::vector<uint16_t> words;

  if (!inputFile.good()) {
    std::cerr << "can not open file " << constants::DATA_FILE << " for reading of real event data" << std::endl;
    return map;
  }

  std::getline(inputFile, line);

  while(!inputFile.eof()){

    //Break when selected number of timeframes is done.
    if(timeframe > constants::NUMBER_TIME_WINDOWS_TO_SIMULATE){
      break;
    }

    uint16_t prev = 0;
    std::list<Sample> list;
    for(int i = 0; i < 1000; i++){
      std::getline(inputFile, line);
      std::istringstream ss(line);
      ss >> sector >> pad >> padnr >> time >> value >> q;

      Sample sample(timeframe, sampleId, 1, value);
      list.push_back(sample);
      sampleId++;

      //Huffman!
      int16_t temp = value;
      int16_t t_res = (temp - prev) + constants::HUFFMAN_PREFIX;
      uint16_t res = t_res;
      words.push_back(res);
      prev = temp;

    }
    for(int i = 0; i < samplePostfix; i++){
      Sample sample;
      sample.timeWindow = timeframe;
      list.push_back(sample);

      //Huffman
      words.push_back(constants::HUFFMAN_PREFIX);
      prev = 0;
    }
    entry.insert(std::pair<int, std::list<Sample>>(count, list));
    count++;

    if(count == constants::SAMPA_NUMBER_INPUT_PORTS * constants::NUMBER_OF_SAMPA_CHIPS){
      map.push_back(entry);
      entry.clear();
      count = 0;
      timeframe++;
    }

  }
  HuffCodeMap codes;
  huffman.CreateTree(words, codes);
  huffman.WriteCodesToFile(constants::HUFFMAN_TREE_FILE_NAME, codes);

  return map;
}

//Uses a gaussian distribution and sends it to the different channels.
void DataGenerator::sendGaussianDistribution(){

  double spacing;
  RandomGenerator randomGenerator;
  std::vector<int> currentOccupancy = getOccupancy();
  for(int i = 0; i < currentOccupancy.size(); i++){
    std::cout << currentOccupancy[i] << std::endl;
  }
  int currentTimeWindow = 1;
  int index = randomGenerator.generate(0, constants::NUMBER_TIME_WINDOWS_TO_SIMULATE - 1);
  std::default_random_engine gen;
  std::normal_distribution<double> dist(calcSpace(currentOccupancy[index]), 0.5);
  spacing = dist(gen);
  int64_t packetCounter = 1;
  int currentSample = 0;

  bool sendSample = true;

  int sendCount = 0;
  int emptyCount = 0;
  std::vector<int> signals;

  //While we still have timewindows to send
  while(currentTimeWindow <= constants::NUMBER_TIME_WINDOWS_TO_SIMULATE)
  {
    //Loop each channel
    for(int i = 0; i < constants::NUMBER_OF_SAMPA_CHIPS * constants::SAMPA_NUMBER_INPUT_PORTS; i++)
    {
      if(sendSample){
        Sample sample(currentTimeWindow, packetCounter, 1, 1, currentOccupancy[index]);
        packetCounter++;
        porter_DG_to_SAMPA[i]->nb_write(sample);
      } else {
        Sample emptySample(currentTimeWindow, packetCounter, 1, 0, currentOccupancy[index]);
        porter_DG_to_SAMPA[i]->nb_write(emptySample);

      }
    }

    //Stupid impl to create peaks of length 3.
    if(sendSample){
      sendCount++;
    } else {
      emptyCount++;
    }
    currentSample++;
    if(sendCount == 3){
      sendCount = 0;
      sendSample = false;
    }
    if(emptyCount >= spacing){
      spacing = dist(gen);
      emptyCount = 0;
      sendSample = true;
    }
    std::cout << "Sending data " << currentSample << '\r';

    //Increments timeWindow
    if(currentSample == constants::NUMBER_OF_SAMPLES_IN_EACH_TIME_WINDOW )//1021 samples
    {
      currentTimeWindow++;
      std::cout << "Current packetCounter: " << packetCounter << " window: " << currentTimeWindow << " occupancy: " << currentOccupancy[index] << endl;

    index = randomGenerator.generate(0, constants::NUMBER_TIME_WINDOWS_TO_SIMULATE - 1);

    dist.param(std::normal_distribution<double>::param_type(calcSpace(currentOccupancy[index]), 0.5));
    currentSample = 0;
    sendCount = 0;
    emptyCount = 0;
    sendSample = true;

  }

  wait((constants::DG_WAIT_TIME), SC_NS);
}
}

//Uses the occupancy number generated by the getOccupancy function to calculate space between peaks.
double DataGenerator::calcSpace(int occ){
  double numberOfSamples = (constants::NUMBER_OF_SAMPLES_IN_EACH_TIME_WINDOW * occ) / 100.0;
  double numberOfPeaks = numberOfSamples / 3.0;
  double numberOfEmptyTimeBins = constants::NUMBER_OF_SAMPLES_IN_EACH_TIME_WINDOW - numberOfSamples;
  return (numberOfEmptyTimeBins / numberOfPeaks);

}

//Creates the set with the distribution of occupancies.
std::vector<int> DataGenerator::getOccupancy(){

  RandomGenerator generator;
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  double mean = 27;
  double sum = 0.0;
  double array[100];

  array[0] = 74.0;
  for(int i = 1; i <= 10; i++){
    array[i] = 44.0;
  }
  for(int i = 11; i < 100; i++){
    array[i] = generator.generate(mean-10, mean+10);
  }
  for(int i = 0; i < 100; i++)
  sum += pow(array[i] - mean, 2.0);

  double varians = sum/100.0;
  double deviation = sqrt(varians);

  std::default_random_engine gen(seed);
  std::normal_distribution<double> dist(mean, deviation);
  std::vector<int> result;
  for(int i = 0; i < constants::NUMBER_TIME_WINDOWS_TO_SIMULATE; i++){
    int o = (int)dist(gen);
    if(o <= 0)
      result.push_back(1);
    else
      result.push_back(o);
  }
  return result;

}

/*
* Writing log data to text file.
*
*/
void DataGenerator::write_log_to_file_sink(int _packetCounter, int _port, int _currentTimeWindow){
  if (constants::DG_GENERATE_OUTPUT)
  {
    std::ofstream outputFile;
    outputFile.open(constants::OUTPUT_FILE_NAME, std::ios_base::app);
    outputFile << sc_time_stamp() << ": " << name() << " Sent packet "
    << _packetCounter << ", to port " << _port << ", current time window: " << _currentTimeWindow
    << std::endl;
    outputFile.close();
  }
}
