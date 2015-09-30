/// @file   write-tpc-altro-mapping.C
/// @author Matthias.Richter@scieq.net
/// @date   2015-07-22
/// @brief  Extract the TPC Altro mapping from raw file and OCDB
///
/// The macro extracts the Altro mapping for all channels from a raw
/// data file.
/// Usage:
/// ls rawdata.root | aliroot -b -q -l write_tpc_altro_mapping.C
/// # replace 'ls rawdata.root' by appropriate command printing
/// # the raw file names
/// # adjust: OCDB Uri, default is "local://.OCDB"
/// #         run number
///
/// Requirements:
/// - The OCDB folder has to be in the current running directory or
///   a link to the OCDB. This default path can be changed in the
///   function parameters below.
/// - Path to a raw file, either as function parameter below or read
///   from standard input.
/// - Run number is in most cases read from the raw file.
///
/// Output: file mapping.dat containing: DDL HWAddress PadRow Pad per Channel
#if defined(__CINT__) && !defined(__MAKECINT__)
{
  gSystem->AddIncludePath("-I$ROOTSYS/include -I$ALICE_ROOT/include");
  TString macroname=gInterpreter->GetCurrentMacroName();
  macroname+="+";
  gROOT->LoadMacro(macroname);
  write_tpc_altro_mapping();
}
#else
#include "AliRawReader.h"
#include "AliTPCRawStreamV3.h"
#include "AliTPCAltroMapping.h"
#include "AliTPCcalibDB.h"
#include "AliCDBManager.h"
#include "AliGRPManager.h"
#include "TGrid.h"
#include "TString.h"
#include <fstream>
#include <iomanip>

void write_tpc_altro_mapping(const char* filename=NULL,
                             const char* cdbURI="local://./OCDB",
                             int         runNo=0)
{
  TGrid* pGrid=NULL;
  TString line;
  int DDLNumber=-1;
  int HWAddress=-1;
  int PadRow=-1;
  int Pad=-1;

  const char* ofilename="mapping.dat";
  std::ofstream output(ofilename);
  if (!output.good()) {
    std::cerr << "can not open file '" << ofilename << "' for writing" << std::endl;
    return;
  }

  if (filename==NULL) {
  line.ReadLine(cin);
  } else {
    line=filename;
  }
  if (filename!=NULL || cin.good()) {
    if (pGrid==NULL && line.BeginsWith("alien://")) {
      pGrid=TGrid::Connect("alien");
      if (!pGrid) return;
    }
    cout << "open file " << " '" << line << "'" << endl;
    AliRawReader* rawreader=AliRawReader::Create(line);
    if (!rawreader) {
      std::cout << "failed to create RawReader" << std::endl;
      return;
    }
    // read the first event of the raw input to fetch the
    // event headers with the run number
    rawreader->NextEvent();
    unsigned runNoFromRawFile=rawreader->GetRunNumber();

    // Set the CDB storage location and run numbr
    AliCDBManager::Instance()->SetDefaultStorage(cdbURI);
    if (runNo>0) {
      AliCDBManager::Instance()->SetRun(runNo);
    } else if (runNoFromRawFile>0) {
      std::cout << "Info: Setting run number from raw file '" << line << "': " << runNoFromRawFile << std::endl;
      AliCDBManager::Instance()->SetRun(runNoFromRawFile);
    } else {
      std::cerr << "Error: can not fetch run number from raw file, please specify in the parameter list" << std::endl;
      return;
    }
    // initialize magnetic field from OCDB configuration
    AliGRPManager grpman;
    grpman.ReadGRPEntry();
    grpman.SetMagField();

    // read the channel mapping from OCDB
    AliTPCAltroMapping** mapping =AliTPCcalibDB::Instance()->GetMapping();
    if (!mapping) {
      std::cerr << "failed to load TPC Altro mapping configuration from OCDB, aborting ..." << std::endl;
      return;
    }

    // now create the input stream
    AliTPCRawStreamV3* inputstream=new AliTPCRawStreamV3(rawreader, (AliAltroMapping**)mapping);
    if (!inputstream) {
      std::cout << "failed to create RawStream" << std::endl;
      return;
    }

    // rewind to start with the first event
    rawreader->RewindEvents();
    int nevents=0;
    int nddls=0;
    while (rawreader->NextEvent()) {
      inputstream->Reset();
      inputstream->SelectRawData("TPC");
      nddls=0;
      while (inputstream->NextDDL()) {
	nddls++;
	DDLNumber=inputstream->GetDDLNumber();
	cout << " reading DDL " << std::setw(4) << DDLNumber
	     << " (" << line << " event " << nevents << ")"
	     << endl;
	int nChannels=0;
	while (inputstream->NextChannel()) {
	  nChannels++;
	  HWAddress=inputstream->GetHWAddress();
	  PadRow=inputstream->GetRow();
	  Pad=inputstream->GetPad();
	  output << std::setw(3) << DDLNumber
		 << std::setw(6) << HWAddress
		 << std::setw(6) << PadRow
		 << std::setw(6) << Pad
		 << std::endl;
	}
	if (nChannels>0) {
	cout << nChannels << " channel(s)"
	     << " in DDL " << std::setw(4) << DDLNumber
	     << " (" << line << " event " << nevents << ")"
	     << endl;
	} else {
	  // don't count as this is an empty event
	  nddls--;
	}
      }
      nevents++;
      if (nddls>0) {
	// stop at the first event with data in the channels
	break;
      }
    }
    if (nevents==0) {
      cout << "info: no events found in " << line << endl;
    } else if (nddls==0) {
      cout << "could not find any channel data to extract the mapping for" << endl;
    }
  }
  output.close();
}
#endif
