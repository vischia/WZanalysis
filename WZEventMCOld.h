#ifndef WZEventMCOld_h
#define WZEventMCOld_h



#ifdef DATA
#define WZBASECLASS WZ2012Data
#include "WZ2012Data.h"
#endif
#ifdef OLDMC
#define WZBASECLASS WZ
#include "WZ.h"
#endif
#ifdef NEWMC
#define WZBASECLASS WZGenEvent
#include "WZGenEvent.h"
#endif
#ifdef TZMC
#define WZBASECLASS TZJets
#include "TZJets.h"
#endif

//Lucija commented it
//#define WZBASECLASS WZGenEvent

#include "TLorentzVector.h"
#include "TH2F.h"
#include <vector>

class RecoLepton : public TLorentzVector {

public:

  RecoLepton(double pt, double eta, double phi,
	     float ch, float id ) {
    SetPtEtaPhiM(pt, eta, phi, 0.);
    charge = ch;
    pdgid  = id;
  };

  float GetScaleFactor();

protected:
  float pdgid;
  float charge;

  static TH2F * MuonSF;
  static TH2F * ElecSF;
};

class WZEventMCOld : public WZBASECLASS
{
public:
  WZEventMCOld(TTree *tree);
  bool passesSelection();

  void ReadEvent();

  void DumpEvent(std::ostream & out);

  void PrintSummary();

  //  bool PassesGenCuts();


  float LeptonPt(int i);
  float LeptonBDT(int i);
  float LeptonCharge(int i);
  float LeptonEta(int i);
  float LeptonPhi(int i);
  int   LeptonPass2012ICHEP(int i);
  float LeptonIso(int i);
  float LeptonIsomva(int i);


  float GetMCWeight();
  float GetTriggerEfficiency();

  //  FinalState GetFinalState(){return final_state;}
  //  PassedSelectionStep GetSelectionLevel() { return selection_level;}
  

//     float pt[leptonNumber]={cWZ->pt1, cWZ->pt2, cWZ->pt3, cWZ->pt4};
//     float bdt[leptonNumber]={cWZ->bdt1, cWZ->bdt2, cWZ->bdt3, cWZ->bdt4};
//     float ch[leptonNumber]={cWZ->ch1, cWZ->ch2, cWZ->ch3, cWZ->ch4};
//     float eta[leptonNumber]={cWZ->eta1, cWZ->eta2, cWZ->eta3, cWZ->eta4};
//     float phi[leptonNumber]={cWZ->phi1, cWZ->phi2, cWZ->phi3, cWZ->phi4};
//     int pass2012ICHEP[leptonNumber]={cWZ->pass2012ICHEP1, cWZ->pass2012ICHEP2, cWZ->pass2012ICHEP3, cWZ->pass2012ICHEP4};
//     float iso[leptonNumber]={cWZ->iso1, cWZ->iso2, cWZ->iso3, cWZ->iso4};
//     float isomva[leptonNumber]={cWZ->isomva1, cWZ->isomva2, cWZ->isomva3, cWZ->isomva4};


//  std::vector<TLorentzVector> genLeptons;


  std::vector<TLorentzVector> recoJets;

  std::vector<RecoLepton>  leptons;

  float NumZ() { return numZ;}
  float NumW() { return numW;}

  double SelectedZPt() {
    return selectedZPt;
  }


protected:

  std::vector<float*> pt;
  std::vector<float*> bdt;
  std::vector<float*> pdgid;
  std::vector<float*> ch;
  std::vector<float*> eta;
  std::vector<float*> phi;
  std::vector<int*> pass2012ICHEP;
  std::vector<float*> iso;
  std::vector<float*> isomva;



  //  FinalState          final_state;
  // PassedSelectionStep selection_level;

  int  wLeptonIndex;
  int  zLeptonsIndex[2];

  
  double selectedZPt;

  float numZ;
  float numW;
  float numMET;
  float num3e;
  float num2e1mu;
  float num1e2mu;
  float num3mu;
  float numMET3e;
  float numMET2e1mu;
  float numMET1e2mu;
  float numMET3mu;


};


#endif // #ifdef WZ_cxx
