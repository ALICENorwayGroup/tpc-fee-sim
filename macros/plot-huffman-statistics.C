/// @file   plot-huffman-statistics.C
/// @author Matthias.Richter@scieq.net
/// @date   2015-09-04
/// @brief  Create plots from the gathered huffman statistsics
///
/// The macro description follows soon ...
/// Usage:
/// aliroot -b -q -l plot-huffman-statistics.C
///
#if defined(__CINT__) && !defined(__MAKECINT__)
{
  gSystem->AddIncludePath("-I$ROOTSYS/include");
  TString macroname=gInterpreter->GetCurrentMacroName();
  macroname+="+g";
  gROOT->LoadMacro(macroname);
  plot_huffman_statistics();
}
#else
#include <vector>
#include <iostream>
#include <fstream>
#include <memory>
#include "TTree.h"
#include "TFile.h"
#include "TCanvas.h"
#include "TLegend.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TSystem.h"
#include "TProfile.h"

void formatHuffmanFactorVsOccupancy(TH1& h, const char* type = "");

struct dataset_t {
  int  type;
  TH1* hHuffmanFactor;
  TH1* hOccupancy;
  TProfile* hHuffmanFactorVsPadrow;
  TProfile* hOccupancyVsPadrow;
};

struct datadefinition_t {
  const char* filename;
  int         type;
  const char* selection;
};

template<typename T>
T* project(T*,
	   TTree& tree,
	   const char* name,
	   const char* expr,
	   const char* cut = "",
	   const char* option = ""
	   )
{
  tree.Project(name, expr, cut, option, tree.GetEntries(), 0);
  TObject* obj = gDirectory->Get(name);
  if (!obj) return NULL;
  std::cout << "Created object '" << obj->GetName() << "(" << obj->GetTitle() << ")" << "' of type " << obj->ClassName() << std::endl;
  obj->Draw();
  return dynamic_cast<T*>(obj);
}

void plot_huffman_statistics()
{
  const char* typeNames[] = {
    "Full Huffman",
    "Full Huffman, Data with common mode effect",
    "Truncated Huffman, Data with common mode effect",
    NULL
  };

  datadefinition_t inputdefinitions[] = {
    {"tpc-raw-channel-stat_1.root", 0, "NFilledTimebins>0"},
    {"tpc-raw-channel-stat_2.root", 1, "NFilledTimebins>0"},
    {"tpc-raw-channel-stat_3.root", 2, "NFilledTimebins>0"},
    {NULL, -1, NULL}
  };
  std::vector<dataset_t> datasets;
  TList histograms;

  TFile output("plots.root", "RECREATE");

  for (int idefinition = 0; inputdefinitions[idefinition].filename != NULL; idefinition++) {
    std::auto_ptr<TFile> input(TFile::Open(inputdefinitions[idefinition].filename));
    if (not input.get() or input->IsZombie()) {
      std::cerr << "can not open " << inputdefinitions[idefinition].filename << std::endl;
      continue;
    }

    const char* key = "huffmanstat";
    TObject* obj = NULL;
    input->GetObject(key, obj);
    if (!obj) {
      std::cerr << "can not find object '" << key << "'" << std::endl;
      continue;
    }

    TTree* tree=dynamic_cast<TTree*>(obj);
    if (!tree) {
      std::cerr << "object '" << key << "' has wrong type, TTree required" << std::endl;
      continue;
    }
    output.cd();
    tree = tree->CloneTree();
    input->Close();

    dataset_t dummy;
    datasets.push_back(dummy);

    dataset_t& dataset = datasets.back();

    TString histoname;
    dataset.type                   = inputdefinitions[idefinition].type;
    histoname.Form("hHuffmanFactor_%d_%d", idefinition, dataset.type);
    dataset.hHuffmanFactor         = project((TH1*)NULL, *tree, histoname.Data(), "HuffmanFactor");
    histoname.Form("hOccupancy_%d_%d", idefinition, dataset.type);
    dataset.hOccupancy             = project((TH1*)NULL, *tree, histoname.Data(), "NFilledTimebins/1024");
    histoname.Form("hHuffmanFactorVsPadrow_%d_%d", idefinition, dataset.type);
    dataset.hHuffmanFactorVsPadrow = project((TProfile*)NULL, *tree, histoname.Data(), "HuffmanFactor:PadRow", inputdefinitions[idefinition].selection, "prof");
    histoname.Form("hOccupancyVsPadrow_%d_%d", idefinition, dataset.type);
    dataset.hOccupancyVsPadrow     = project((TProfile*)NULL, *tree, histoname.Data(), "NFilledTimebins/1024:PadRow",inputdefinitions[idefinition].selection,"prof");

    if (dataset.hHuffmanFactor) histograms.Add(dataset.hHuffmanFactor);
    if (dataset.hOccupancy) histograms.Add(dataset.hOccupancy);
    if (dataset.hHuffmanFactorVsPadrow) histograms.Add(dataset.hHuffmanFactorVsPadrow);
    if (dataset.hOccupancyVsPadrow) histograms.Add(dataset.hOccupancyVsPadrow);
  }

  ///////////////////////////////////////////////////////////////////////////
  //
  // create the derived histograms
  //
  // Both Occupancy vs Padrow and HuffmanFactor vs Padrow can be plotted as
  // profile histogram from the statistics tree. In order to combine the two
  // the average values are extracted from the profile histogram through a
  // projection in X. Then the values are filled into a new tree and the
  // relation HuffmanFactor vs Occupancy can be plotted from that.

  // step 1: fill intermediate tree
  int Type = -1;
  int Row = -1;
  float Occupancy = 0.;
  float huffmanfactor = 0.;

  TTree *datatree=new TTree("rawhuffstat","TPC raw huffman statistic");
  datatree->Branch("Type"          , &Type          , "Type/I");
  datatree->Branch("Row"           , &Row           , "Row/I");
  datatree->Branch("Occupancy"     , &Occupancy     , "Occupancy/F");
  datatree->Branch("huffmanfactor" , &huffmanfactor , "huffmanfactor/F");

  int nTypes=-1;
  for (std::vector<dataset_t>::iterator dataset = datasets.begin();
       dataset != datasets.end(); dataset++) {
    if (dataset->type > nTypes) nTypes=dataset->type;
    Type=dataset->type;
    
    TH1* hfproj = dataset->hHuffmanFactorVsPadrow->ProjectionX();
    TH1* occproj = dataset->hOccupancyVsPadrow->ProjectionX();

    int nbins = hfproj->GetNbinsX();
    if (nbins != occproj->GetNbinsX()) {
      std::cerr << "number of bins does not match" << std::endl;
      continue;
    }

    for (Row = 1; Row < nbins; Row++) {
      huffmanfactor = hfproj->GetBinContent(Row);
      Occupancy     = occproj->GetBinContent(Row);
      datatree->Fill();
    }
  }

  // step 2: create plots for each type
  output.cd();
  histograms.Write();

  nTypes++;
  std::vector<TProfile*> hfvsocc(nTypes, NULL);

  for (int itype = 0; itype < nTypes; itype++) {
    TString histoname;
    TString selection;
    histoname.Form("hHuffmanFactorVsOccupancy_%d", itype);
    selection.Form("Type==%d && Occupancy>0.", itype);
    hfvsocc[itype] = project((TProfile*)NULL, *datatree, histoname.Data(), "huffmanfactor:100*Occupancy", selection.Data(), "prof");
    if (hfvsocc[itype]) {
      formatHuffmanFactorVsOccupancy(*hfvsocc[itype]);
      hfvsocc[itype]->SetErrorOption();
      hfvsocc[itype]->Write();
    }
  }

  // step 3: combine plots in a canvas
  int firstcolor = 1;
  TCanvas c1("ccompilation", "Huffman Compression vs. Occupancy");
  TLegend *legend = new TLegend(.3,.3,.7,.6);
  legend->SetBorderSize(0);
  legend->SetTextSize(.035);
  c1.cd();
  bool drawSame = false;
  for (int itype = 0; itype < nTypes; itype++) {
    if (hfvsocc[itype]) {
      hfvsocc[itype]->SetLineColor(itype + firstcolor);
      hfvsocc[itype]->SetMarkerColor(itype + firstcolor);
      hfvsocc[itype]->Draw(drawSame?"same":"");
      drawSame = true;
      legend->AddEntry(hfvsocc[itype], typeNames[itype], "lp");
    }
  }  
  legend->Draw();

  output.cd();
  c1.Write();
  output.Close();
}

void formatHuffmanFactorVsOccupancy(TH1& h, const char* type)
{
  TString name("Huffman Compression vs. Occupancy");
  if (type) {
    name += " ("; name += type; name += ")";
  }
  h.SetTitle("Huffman Compression vs. Occupancy");
  h.SetLineColor(1);
  h.GetXaxis()->SetTitle("Occupancy [%]");
  h.GetXaxis()->SetLabelFont(42);
  h.GetXaxis()->SetLabelSize(0.045);
  h.GetXaxis()->SetTitleSize(0.045);
  h.GetXaxis()->SetTitleFont(42);
  h.GetYaxis()->SetTitle("Huffman factor");
  h.GetYaxis()->SetLabelFont(42);
  h.GetYaxis()->SetLabelSize(0.045);
  h.GetYaxis()->SetTitleSize(0.045);
  h.GetYaxis()->SetTitleFont(42);
  h.SetMarkerStyle(2);
}

#endif
