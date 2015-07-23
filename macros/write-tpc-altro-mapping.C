/// @file   write-tpc-altro-mapping.C
/// @author Matthias.Richter@scieq.net
/// @date   2015-07-22
/// @brief  Extract the TPC Altro mapping from raw file and OCDB
///
/// The macro extracts the Altro mapping for all channels from a raw
/// data file.
/// Usage:
/// ls rawdata*.root | aliroot -b -q -l write_tpc_altro_mapping.C
/// # replace 'ls rawdata*.root' by appropriate command printing
/// # the raw file names
/// # adjust: OCDB Uri, default is "local://.OCDB"
/// #         run number
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

void write_tpc_altro_mapping()
{
  // Set the CDB storage location
  AliCDBManager::Instance()->SetDefaultStorage("local://./OCDB");
  AliCDBManager::Instance()->SetRun(167808);
  AliGRPManager grpman;
  grpman.ReadGRPEntry();
  grpman.SetMagField();

  AliTPCAltroMapping** mapping =AliTPCcalibDB::Instance()->GetMapping();
  if (!mapping) {
    std::cerr << "failed to load TPC Altro mapping, aborting ..." << std::endl;
    return;
  }

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

  line.ReadLine(cin);
  if (cin.good()) {
    if (pGrid==NULL && line.BeginsWith("alien://")) {
      pGrid=TGrid::Connect("alien");
      if (!pGrid) return;
    }
    cout << "open file " << " '" << line << "'" << endl;
    AliRawReader* rawreader=AliRawReader::Create(line);
    AliTPCRawStreamV3* inputstream=new AliTPCRawStreamV3(rawreader, (AliAltroMapping**)mapping);
    if (!rawreader || !inputstream) {
      return;
    }
    rawreader->RewindEvents();
    if (!rawreader->NextEvent()) {
      cout << "info: no events found in " << line << endl;
    } else {
      inputstream->Reset();
      inputstream->SelectRawData("TPC");
      while (inputstream->NextDDL()) {
	DDLNumber=inputstream->GetDDLNumber();
	cout << " reading DDL " << std::setw(4) << DDLNumber
	     << " (" << line << ")"
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
	cout << nChannels << " channel(s)"
	     << " in DDL " << std::setw(4) << DDLNumber
	     << " (" << line << ")"
	     << endl;
      }
    }
  }
  output.close();
}
#endif
