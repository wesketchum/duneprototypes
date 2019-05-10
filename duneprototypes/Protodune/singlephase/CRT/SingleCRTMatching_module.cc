////////////////////////////////////////////////////////////////////////
// Class:       SingleCRTMatching
// Plugin Type: analyzer (art v2_11_02)
// File:        SingleCRTMatching_module.cc
//
// Written by: Richard Diurba
// Adapted from MCC Code by: Arbin Timilsina 
// CRT Trigger Architecture by: Andrew Olivier 
////////////////////////////////////////////////////////////////////////

//Framework includes
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "canvas/Utilities/InputTag.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "canvas/Persistency/Common/Assns.h"
#include "canvas/Persistency/Common/FindManyP.h"
#include "art_root_io/TFileService.h"

//LArSoft includes

#include "lardataobj/Simulation/AuxDetSimChannel.h"
#include "larcore/Geometry/Geometry.h"
#include "nusimdata/SimulationBase/MCParticle.h"
#include "nutools/ParticleNavigation/ParticleList.h"

#include "larsim/MCCheater/BackTrackerService.h"
#include "lardataobj/RecoBase/Track.h"
#include "lardataobj/RecoBase/Hit.h"
#include "larsim/MCCheater/ParticleInventoryService.h"

#include "lardata/DetectorInfoServices/DetectorPropertiesService.h"
#include "lardata/DetectorInfoServices/DetectorClocksService.h"

#include "dune/Protodune/singlephase/CTB/data/pdspctb.h"
#include "lardataobj/RawData/RDTimeStamp.h"
#include "lardataobj/RecoBase/OpFlash.h"
#include "lardataobj/RecoBase/OpHit.h"

//Local includes
#include "dunetpc/dune/Protodune/singlephase/CRT/data/CRTTrigger.h"

//ROOT includes
#include "TH1.h"
#include "TH2.h"
#include "TCanvas.h"
#include "TImage.h"
#include "TTree.h"
#include "TH1D.h"
#include "TStyle.h"
#include "TString.h"

//c++ includes
#include <numeric> //std::accumulate was moved from <algorithm> to <numeric> in c++14
#include <iostream>
#include <cmath>
using namespace std;   // Namespaces established to make life easier
using namespace ROOT::Math;
namespace CRT {
  class SingleCRTMatching;
}


class CRT::SingleCRTMatching: public art::EDAnalyzer {
  public: // Setup functions and variables
    explicit SingleCRTMatching(fhicl::ParameterSet
      const & p);
  std::string fTrackModuleLabel = "pandoraTrack";
  std::string fopModuleLabel= "ophit";
  // The compiler-generated destructor is fine for non-base
  // classes without bare pointers or other resource use.
  // Plugins should not be copied or assigned.
  SingleCRTMatching(SingleCRTMatching
    const & ) = delete;
  SingleCRTMatching(SingleCRTMatching && ) = delete;
  SingleCRTMatching & operator = (SingleCRTMatching
    const & ) = delete;
  SingleCRTMatching & operator = (SingleCRTMatching && ) = delete;
  void analyze(art::Event
    const & e) override;
// Declare functions and variables for validation
  bool moduleMatcherData(int module1, int module2);
  bool moduleMatcherMCC(int module1, int module2);
  void beginJob() override;
  void endJob() override;
  double setAngle(double angle);
int moduletoCTB(int module2, int module1);
  int nEvents = 0;
  int nHaloMuons = 0;
  private: ofstream logFile;

  //Parameters for reading in CRT::Triggers and associated AuxDetSimChannels.
  art::InputTag fCRTLabel; //Label for the module that produced 
  art::InputTag fCTBLabel;
  TTree * fCRTTree;
    bool fMCCSwitch;
    bool fModuleSwitch;
    int fADCThreshold;
    int fModuletoModuleTimingCut;
    int fFronttoBackTimingCut;
    int fOpCRTTDiffCut;


    double dotCos;
    int adcX, adcY;
    double X_CRT, Y_CRT, Z_CRT;
    double trackX1, trackX2, trackY1, trackY2, trackZ1, trackZ2;
    int moduleX, moduleY; 
    int stripX, stripY;
    double deltaX, deltaY;
    double CRTT0;
    double opCRTTDiff;
  typedef struct // Structures for arrays to move hits from raw to reco to validation
  {

    int channel;
    int module;
    int channelGeo;
    int adc;
    int triggerTime;
  }
  tempHits;

  typedef struct {
    int tempId;	
    int adcX;
    int adcY;

    double hitPositionX;
    double hitPositionY;
    double hitPositionZ;
    double timeAvg;
    int moduleX;
    int moduleY;
    int stripX;
    int stripY;
  }
  recoHits;

  typedef struct // These are displacement metrics for track and hit reco
  {
    int tempId;
    int CRTTrackId;
    int recoId;
    int adcX1;
    int adcY1;
    double deltaX;
    double deltaY;
    double averageSignedDistanceXY;
    double averageSignedDistanceYZ;
    double averageSignedDistanceXZ;
    double averageSignedDistance;
    double dotProductCos;
    double X1;
    double Y1;
    double Z1;
    int trackID;
    TVector3 trackStartPosition;
    TVector3 trackEndPosition;
    int moduleX1, moduleY1;
    int stripX1, stripY1;
    double flashTDiff;
    double timeAvg; 
  }
  tracksPair;

  struct removePairIndex // Struct to remove tracks after sorting
  {
    const tracksPair tracksPair1;
    removePairIndex(const tracksPair & tracksPair0): tracksPair1(tracksPair0) {}

    bool operator()(const tracksPair & tracksPair2) {
      return (tracksPair1.recoId == tracksPair2.recoId || tracksPair1.tempId == tracksPair2.tempId);
    }
  };

  struct sortPair // Struct to sort to find best CRT track for TPC track
  {
    bool operator()(const tracksPair & pair1,
      const tracksPair & pair2) {
     
	return (fabs(pair1.dotProductCos)>fabs(pair2.dotProductCos));

  }
  };
  std::vector < recoHits > primaryHits_F;
  std::vector < recoHits > primaryHits_B;

  std::vector < tempHits > tempHits_F;
  std::vector < tempHits > tempHits_B;
  std::vector < tracksPair > tracksPair_F;
  std::vector < tracksPair > tracksPair_B;
};

CRT::SingleCRTMatching::SingleCRTMatching(fhicl::ParameterSet
    const & p):
  EDAnalyzer(p), fCRTLabel(p.get < art::InputTag > ("CRTLabel")), fCTBLabel(p.get<art::InputTag>("CTBLabel")) {
    consumes < std::vector < CRT::Trigger >> (fCRTLabel);
    consumes < std::vector < art::Assns < sim::AuxDetSimChannel, CRT::Trigger >>> (fCRTLabel); // CRT art consumables
  fMCCSwitch=(p.get<bool>("MCC"));
  }


// Function to match CRT modules below is for MCC and the 2nd is for data
bool CRT::SingleCRTMatching::moduleMatcherMCC(int module1, int module2) {
  // Function checking if two hits could reasonably be matched into a 2D hit
  if ((module1 == 0 && (module2 == 5 || module2 == 4)) || (module1 == 12 && (module2 == 5 || module2 == 4)) || (module1 == 16 && (module2 == 20 || module2 == 21)) || (module1 == 28 && (module2 == 20 || module2 == 21)) || (module1 == 1 && (module2 == 6 || module2 == 7)) || (module1 == 13 && (module2 == 6 || module2 == 7)) || (module1 == 17 && (module2 == 22 || module2 == 23)) || (module1 == 29 && (module2 == 22 || module2 == 23)) || (module1 == 2 && (module2 == 10 || module2 == 11)) || (module1 == 14 && (module2 == 10 || module2 == 11)) || (module1 == 19 && (module2 == 24 || module2 == 25)) || (module1 == 31 && (module2 == 24 || module2 == 25)) || (module1 == 3 && (module2 == 8 || module2 == 9)) || (module1 == 15 && (module2 == 8 || module2 == 9)) || (module1 == 18 && (module2 == 26 || module2 == 27)) || (module1 == 30 && (module2 == 26 || module2 == 27))) return 1;
  else return 0;

}

// Function to match CRT modules below is for MCC and the 2nd is for data
bool CRT::SingleCRTMatching::moduleMatcherData(int module1, int module2) {
  // Function checking if two hits could reasonably be matched into a 2D hit
  if ((module1 == 0 && (module2 == 5 || module2 == 4)) || (module1 == 12 && (module2 == 5 || module2 == 4)) || (module1 == 16 && (module2 == 20 || module2 == 21)) || (module1 == 28 && (module2 == 20 || module2 == 21)) || (module1 == 1 && (module2 == 6 || module2 == 7)) || (module1 == 13 && (module2 == 6 || module2 == 7)) || (module1 == 17 && (module2 == 22 || module2 == 23)) || (module1 == 29 && (module2 == 22 || module2 == 23)) || (module1 == 2 && (module2 == 10 || module2 == 11)) || (module1 == 14 && (module2 == 10 || module2 == 11)) || (module1 == 19 && (module2 == 24 || module2 == 25)) || (module1 == 31 && (module2 == 24 || module2 == 25)) || (module1 == 3 && (module2 == 8 || module2 == 9)) || (module1 == 15 && (module2 == 8 || module2 == 9)) || (module1 == 18 && (module2 == 26 || module2 == 27)) || (module1 == 30 && (module2 == 26 || module2 == 27))) return 1;
  else return 0;

}


int CRT::SingleCRTMatching::moduletoCTB(int module2, int module1){
  if (module1 == 13 && module2 == 6 ) return 15;
  else if (module1 == 13 &&  module2 == 7) return 10;
  else if (module1 == 1 &&  module2 == 6) return 8;
  else if (module1 == 1 &&  module2 == 7) return 9;
  else if (module1 == 16 &&  module2 == 20) return 4;
  else if (module1 == 16 &&  module2 == 21) return 13;
  else if (module1 == 28 &&  module2 == 20) return 3;
  else if (module1 == 28 &&  module2 == 21) return 2;
  else if (module1 == 29 &&  module2 == 22) return 1;
  else if (module1 == 29 &&  module2 == 23) return 0;
  else if (module1 == 17 &&  module2 == 22) return 12;
  else if (module1 == 17 &&  module2 == 23) return 11;
  else if (module1 == 0  &&  module2 == 5) return 7;
  else if (module1 == 0 &&  module2 == 4) return 6;
  else if (module1 == 12  &&  module2 == 5) return 14;
  else if (module1 == 12 &&  module2 == 4) return 5;
  else if (module1 == 3 &&  module2 == 8) return 25;
  else if (module1 == 3 &&  module2 == 9) return 24;
  else if (module1 == 15 &&  module2 == 8) return 26;
  else if (module1 == 15 &&  module2 == 9) return 31;
  else if (module1 == 18 &&  module2 == 26) return 27;
  else if (module1 == 18 &&  module2 == 27) return 28;
  else if (module1 == 30 &&  module2 == 26) return 16;
  else if (module1 == 30 &&  module2 == 27) return 17;
  else if (module1 == 31 &&  module2 == 24) return 18;
  else if (module1 == 31 &&  module2 == 25) return 19;
  else if (module1 == 19 &&  module2 == 24) return 29;
  else if (module1 == 19 &&  module2 == 25) return 20;
  else if (module1 == 14 &&  module2 == 10) return 30;
  else if (module1 == 14 &&  module2 == 11) return 21;
  else if (module1 == 2 &&  module2 == 10) return 23;
  else if (module1 == 2 &&  module2 == 11) return 22;
  else return -1;
}

double CRT::SingleCRTMatching::setAngle(double angle) {
  if (angle < 0) {
    angle += 3.14159265359;
  }
  angle *= 180.00 / 3.14159265359;
  return angle;
}


void CRT::SingleCRTMatching::analyze(art::Event
  const & event) // Analysis module
{

  if (fMCCSwitch){
    fModuleSwitch=1;
    fADCThreshold=800;
    fModuletoModuleTimingCut=4;
    fFronttoBackTimingCut=100;
    fOpCRTTDiffCut=200;
    
}
  else {
    fModuleSwitch=0;
    fADCThreshold=20;
    fModuletoModuleTimingCut=5;
    fFronttoBackTimingCut=8;
    fOpCRTTDiffCut=1000000;


}

   if(!fMCCSwitch){
   art::ValidHandle<std::vector<raw::RDTimeStamp>> timingHandle = event.getValidHandle<std::vector<raw::RDTimeStamp>>("timingrawdecoder:daq");
   //const auto& pdspctbs = event.getValidHandle<std::vector<raw::ctb::pdspctb>>(fCTB_tag);

	const raw::RDTimeStamp& timeStamp = timingHandle->at(0);
	if(timeStamp.GetFlags()!= 13) return;
  }


  int nHits = 0;
  art::ServiceHandle < cheat::BackTrackerService > backTracker;
  art::ServiceHandle < cheat::ParticleInventoryService > partInventory;


  primaryHits_F.clear();
  primaryHits_B.clear();
  tracksPair_F.clear();
  tracksPair_B.clear();
  tempHits_F.clear();
  tempHits_B.clear(); // Arrays to compile hits and move them through
  primaryHits_F.clear();
    //allTracksPair.clear();
  logFile.open("ProtoDUNE.log"); // Logfile I don't use right now

  //Get triggers
  cout << "Getting triggers" << endl;
  const auto & triggers = event.getValidHandle < std::vector < CRT::Trigger >> (fCRTLabel);

  art::FindManyP < sim::AuxDetSimChannel > trigToSim(triggers, event, fCRTLabel);

  //Get a handle to the Geometry service to look up AuxDetGeos from module numbers
  art::ServiceHandle < geo::Geometry > geom;

  //Mapping from channel to trigger
  std::unordered_map < size_t, double > prevTimes;
  int hitID = 0;
  cout << "Looking for hits in Triggers" << endl;

  for (const auto & trigger: * triggers) {
    const auto & hits = trigger.Hits();
    for (const auto & hit: hits) { // Collect hits on all modules
	//cout<<hits.size()<<','<<hit.ADC()<<endl;
      if (hit.ADC() > fADCThreshold) { // Keep if they are above threshold

        tempHits tHits;
	if (!fMCCSwitch){
	int stripChannel=hit.Channel();
	if (hit.Channel()<32) stripChannel=hit.Channel()*2;
	else stripChannel=2*(hit.Channel()-32)+1;
	cout<<stripChannel<<endl;
        tHits.module = trigger.Channel(); // Values to add to array
        tHits.channelGeo = stripChannel;
	tHits.channel=hit.Channel();
        tHits.adc = hit.ADC();
	tHits.triggerTime=trigger.Timestamp();
	}
	else{
        tHits.module = trigger.Channel(); // Values to add to array
        tHits.channelGeo = hit.Channel();
	tHits.channel=hit.Channel();
        tHits.adc = hit.ADC();
	tHits.triggerTime=trigger.Timestamp();
	}
	 //cout<<trigger.Channel()<<','<<hit.Channel()<<','<<hit.ADC()<<endl;
        nHits++;

        const auto & trigGeo = geom -> AuxDet(trigger.Channel()); // Get geo  
        const auto & csens = trigGeo.SensitiveVolume(hit.Channel());
        const auto center = csens.GetCenter();
        if (center.Z() < 100) tempHits_F.push_back(tHits); // Sort F/B from Z
        else tempHits_B.push_back(tHits);
        hitID++;
      }
    }
  }

  cout << "Hits compiled for event: " << nEvents << endl;
  cout << "Number of Hits above Threshold:  " << hitID << endl;

  for (unsigned int f = 0; f < tempHits_F.size(); f++) {
    for (unsigned int f_test = 0; f_test < tempHits_F.size(); f_test++) {
     const auto & trigGeo = geom -> AuxDet(tempHits_F[f].module);
      const auto & trigGeo2 = geom -> AuxDet(tempHits_F[f_test].module);
	int flipChannel=tempHits_F[f].channelGeo;
	int flipX=1;
	if (tempHits_F[f].module==21 && !fMCCSwitch){flipX=-1; flipChannel=flipChannel^63;}
      const auto & hit1Geo = trigGeo.SensitiveVolume(flipChannel);
      const auto hit1Center = hit1Geo.GetCenter();
      //const auto hit1Center = trigGeo.GetCenter();
      // Create 2D hits from geo of the Y and X modules
	flipChannel=tempHits_F[f_test].channelGeo;
	int flipY=1;
	if (!fMCCSwitch && (tempHits_F[f_test].module==13 || tempHits_F[f_test].module==1)) {flipY=-1; flipChannel=flipChannel^63;}
	//cout<<"Channel flip: "<<flipChannel<<','<<tempHits_F[f_test].channelGeo;
       const auto & hit2Geo = trigGeo2.SensitiveVolume(flipChannel);
	//const auto hit2Center = hit2Geo.GetCenter();
      const auto hit2Center = hit2Geo.GetCenter();
      bool moduleMatched;
      if(fModuleSwitch) moduleMatched=moduleMatcherMCC(tempHits_F[f_test].module, tempHits_F[f].module);
      else moduleMatched=moduleMatcherData(tempHits_F[f_test].module, tempHits_F[f].module);

      if (moduleMatched) {
        // Get the center of the hits (CRT_Res=2.5 cm)

        double hitX = hit1Center.X();
	for (unsigned int a = 0; a < tempHits_F.size(); a++)
	{
	if(tempHits_F[a].module==tempHits_F[f].module && (tempHits_F[a].channelGeo-flipX)==tempHits_F[f].channelGeo) hitX=hit1Center.X()+1.25;
	}
	double hitYPrelim=hit2Center.Y();
	for (unsigned int a = 0; a < tempHits_F.size(); a++)
	{
	if(tempHits_F[a].module==tempHits_F[f_test].module && (tempHits_F[a].channelGeo-flipY)==tempHits_F[f_test].channelGeo) hitYPrelim=hit2Center.Y()+1.25;
	}
	if(!fMCCSwitch) hitYPrelim=hitYPrelim-50+20;
	double hitY=hitYPrelim;
        double hitZ = (hit1Center.Z() + hit2Center.Z()) / 2.f;

        recoHits rHits;
        rHits.adcX=tempHits_F[f].adc;
	rHits.adcY=tempHits_F[f_test].adc;
        rHits.hitPositionX = hitX;
        rHits.hitPositionY = hitY;
        rHits.hitPositionZ = hitZ;
	rHits.moduleX=tempHits_F[f].module;
	rHits.moduleY=tempHits_F[f_test].module;

	rHits.stripX=tempHits_F[f].channel;
	rHits.stripY=tempHits_F[f_test].channel;
	rHits.timeAvg = (tempHits_F[f_test].triggerTime+tempHits_F[f].triggerTime)/2.0;
	if (fabs(tempHits_F[f_test].triggerTime-tempHits_F[f].triggerTime)<fModuletoModuleTimingCut) primaryHits_F.push_back(rHits); // Add array
//	primaryHits_F.push_back(rHits);  
    }
    }
  }
  for (unsigned int f = 0; f < tempHits_B.size(); f++) {
    for (unsigned int f_test = 0; f_test < tempHits_B.size(); f_test++) { // Same as above but for back CRT
	int channelFlipCheck=tempHits_B[f].module;
	if (!fMCCSwitch){
	if (channelFlipCheck==8) channelFlipCheck=11;
	else if (channelFlipCheck==11) channelFlipCheck=8;
         else if (channelFlipCheck==10) channelFlipCheck=9;
	else if (channelFlipCheck==9) channelFlipCheck=10;

	else if (channelFlipCheck==26) channelFlipCheck=25;
        else if (channelFlipCheck==25) channelFlipCheck=26;
	else if (channelFlipCheck==24) channelFlipCheck=27;
	else if (channelFlipCheck==27) channelFlipCheck=24;
	}
     int flipX=1;
     int flipChannel=tempHits_B[f].channelGeo;
     if (!fMCCSwitch && (tempHits_B[f].module==25 || tempHits_B[f].module==11 || tempHits_B[f].module==24 || tempHits_B[f].module==10)){flipX=-1; flipChannel=flipChannel^63;}
     //if (tempHits_B[f].module==27 || tempHits_B[f].module==26 || tempHits_B[f].module==25 || tempHits_B[f].module==24) flipChannel=flipChannel^63; 
      const auto & trigGeo = geom -> AuxDet(channelFlipCheck);
      const auto & trigGeo2 = geom -> AuxDet(tempHits_B[f_test].module);
      const auto & hit1Geo = trigGeo.SensitiveVolume(flipChannel);
      const auto hit1Center = hit1Geo.GetCenter();
      //const auto hit2Center = trigGeo2.GetCenter();
      //const auto hit1Center=trigGeo.GetCenter();
	int flipY=1;
	 flipChannel=tempHits_B[f_test].channelGeo;
	if (!fMCCSwitch && (tempHits_B[f_test].module==2 || tempHits_B[f_test].module==3  || tempHits_B[f_test].module==14 || tempHits_B[f_test].module==15)) {flipY=-1; flipChannel=flipChannel^63;}
      const auto & hit2Geo = trigGeo2.SensitiveVolume(flipChannel);
      const auto hit2Center = hit2Geo.GetCenter();
      bool moduleMatched;
      if(fModuleSwitch) moduleMatched=moduleMatcherMCC(tempHits_B[f_test].module, tempHits_B[f].module);
      else moduleMatched=moduleMatcherData(tempHits_B[f_test].module, tempHits_B[f].module);

      if (moduleMatched) {

        double hitX = hit1Center.X();
	for (unsigned int a = 0; a < tempHits_B.size(); a++)
	{
	if(tempHits_B[a].module==tempHits_B[f].module && (tempHits_B[a].channelGeo-flipX)==tempHits_B[f].channelGeo) hitX=hit1Center.X()+1.25;
	}
	
        double hitYPrelim = hit2Center.Y();
	
	for (unsigned int a = 0; a < tempHits_B.size(); a++)
	{
	if(tempHits_B[a].module==tempHits_B[f_test].module && (tempHits_B[a].channel-flipY)==tempHits_B[f_test].channel) hitYPrelim=hit2Center.Y()+1.25;
	}
	double hitY=hitYPrelim;
	if (!fMCCSwitch) hitY=hitYPrelim-150+35;
        double hitZ = (hit1Center.Z() + hit2Center.Z()) / 2.f;

        recoHits rHits;
        rHits.adcX=tempHits_B[f].adc;
	rHits.adcY=tempHits_B[f_test].adc;
        rHits.hitPositionX = hitX;
        rHits.hitPositionY = hitY;
        rHits.hitPositionZ = hitZ;
	rHits.moduleX=tempHits_B[f].module;
	rHits.moduleY=tempHits_B[f_test].module;
	rHits.stripX=tempHits_B[f].channel;
	rHits.stripY=flipChannel;
	rHits.timeAvg = (tempHits_B[f_test].triggerTime+tempHits_B[f].triggerTime)/2.0;
       if (fabs(tempHits_B[f_test].triggerTime-tempHits_B[f].triggerTime)<fModuletoModuleTimingCut) primaryHits_B.push_back(rHits); 
	 }
    }
  }
	
        int pixel0 = -1;
        int pixel1 = -1;
	if (!fMCCSwitch)
	{
       const auto& pdspctbs = *event.getValidHandle<std::vector<raw::ctb::pdspctb>>(fCTBLabel);
       std::vector<int> uS, dS;
	const size_t npdspctbs = pdspctbs.size();
	for(size_t j=0;j<npdspctbs;++j)
	  {
	    const std::vector<raw::ctb::Trigger> HLTriggers = pdspctbs[j].GetHLTriggers();
	    const std::vector<raw::ctb::ChStatus> chs = pdspctbs[j].GetChStatusAfterHLTs();
for (size_t k=0; k<HLTriggers.size(); ++k)
	      { 
		//cout<<chs[k].timestamp<<endl;
		int num = chs[k].crt;
		//cout<<num<<endl;
	
        const std::string binary = std::bitset<32>(num).to_string();	
	const auto crtmask=chs[k].crt;
         pixel0 = -1;
         pixel1 = -1;
	//cout<<crtmask<<endl;
        for (int i = 0; i<32; ++i){
          if (crtmask & (1<<i)){
            if (i<16){
              pixel0 = i;
            }
            else {
              pixel1 = i;
            }
          }
   	}
        if (pixel0!=-1 && pixel1!=1) {
	cout<<nEvents<<" TJYang Pixels: "<<pixel0<<","<<pixel1<<endl;
	}
	else return;
	      }
	  }
	}
	
  // Reconstruciton information
  art::Handle < vector < recob::Track > > trackListHandle;
  vector < art::Ptr < recob::Track > > trackList;
  if (event.getByLabel(fTrackModuleLabel, trackListHandle)) {
    art::fill_ptr_vector(trackList, trackListHandle);
  }
	art::Handle< std::vector<recob::OpHit> > opListHandle;
	std::vector<art::Ptr<recob::OpHit> > opHitList;

	if (event.getByLabel(fopModuleLabel, opListHandle))
	    {
		art::fill_ptr_vector(opHitList, opListHandle);
	    }
  int nTracksReco = trackList.size();
  art::FindManyP < recob::Hit > hitsFromTrack(trackListHandle, event, fTrackModuleLabel);
  int tempId = 0;
  for (int iRecoTrack = 0; iRecoTrack < nTracksReco; ++iRecoTrack) {
    if (primaryHits_F.size()<1) break;
// Get track positions and find angles
 //  if (fMCCSwith==1){
    double trackStartPositionZ = trackList[iRecoTrack]->Vertex().Z();
    double trackEndPositionZ = trackList[iRecoTrack] -> End().Z();

    double trackStartPositionX = trackList[iRecoTrack]->Vertex().X();
    double trackStartPositionY = trackList[iRecoTrack]->Vertex().Y();


    double trackEndPositionX = trackList[iRecoTrack] -> End().X();
    double trackEndPositionY = trackList[iRecoTrack] -> End().Y();
    if (trackStartPositionZ>trackEndPositionZ){
    trackEndPositionZ = trackList[iRecoTrack]->Vertex().Z();
    trackStartPositionZ = trackList[iRecoTrack] -> End().Z();
    trackEndPositionX = trackList[iRecoTrack]->Vertex().X();
    trackEndPositionY = trackList[iRecoTrack]->Vertex().Y();


    trackStartPositionX = trackList[iRecoTrack] -> End().X();
    trackStartPositionY = trackList[iRecoTrack] -> End().Y();
    }
 if ((trackEndPositionZ>90 && trackEndPositionZ < 300 && trackStartPositionZ <50)) {
      for (unsigned int iHit_F = 0; iHit_F < primaryHits_F.size(); iHit_F++) {
	if (!fMCCSwitch && moduletoCTB(primaryHits_F[iHit_F].moduleX, primaryHits_F[iHit_F].moduleY)!=pixel0) continue;
        double X1 = primaryHits_F[iHit_F].hitPositionX;

        double Y1 = primaryHits_F[iHit_F].hitPositionY;

        double Z1 = primaryHits_F[iHit_F].hitPositionZ;

	// Make metrics for a CRT pair to compare later
	TVector3 trackStart(trackStartPositionX, trackStartPositionY, trackStartPositionZ);
	TVector3 trackEnd(trackEndPositionX, trackEndPositionY, trackEndPositionZ);
	TVector3 v1(X1,Y1,Z1);
	TVector3 v2(trackStartPositionX, trackStartPositionY, trackStartPositionZ);

            TVector3 v4(trackStartPositionX,
                        trackStartPositionY,
                        trackStartPositionZ);
            TVector3 v5(trackEndPositionX,
                        trackEndPositionY,
                        trackEndPositionZ);
	TVector3 trackVector = (v5-v4).Unit();
	TVector3 hitVector=(v2-v1).Unit();




              double predictedHitPositionY1 = (v1.Z()-v5.Z())/(v4.Z()-v5.Z())*(v4.Y()-v5.Y())+v5.Y();


              double predictedHitPositionX1 = (v1.Z()-v5.Z())/(v4.Z()-v5.Z())*(v4.X()-v5.X())+v5.X();

	double dotProductCos=trackVector*hitVector;

        double deltaX1 = (predictedHitPositionX1-X1);

	double deltaX=(deltaX1);

        double deltaY1 = (predictedHitPositionY1-Y1);

	double deltaY=deltaY1;
	double minTimeDifference=999999.99;
        for (unsigned int iFlash = 0; iFlash < opHitList.size(); iFlash++)
                {
			/*
                    if (flashList[iFlash].PE() < 100)
                        {
                            continue;
                        }
		   */

                    double flashTime = opHitList[iFlash]->PeakTime();
                    double timeDifference = primaryHits_F[iHit_F].timeAvg - flashTime*1000;
		
                    if(fabs(timeDifference) < fabs(minTimeDifference))
                        {
                            minTimeDifference = timeDifference;
                        }
                }

         tracksPair tPair;
        tPair.tempId = tempId;
        tPair.CRTTrackId = iHit_F;
        tPair.recoId = iRecoTrack;

	tPair.deltaX=deltaX;
        tPair.deltaY=deltaY;
        tPair.dotProductCos=dotProductCos;

        tPair.moduleX1 = primaryHits_F[iHit_F].moduleX;
        tPair.moduleY1 = primaryHits_F[iHit_F].moduleY;

        tPair.adcX1=primaryHits_F[iHit_F].adcX;
        tPair.adcY1=primaryHits_F[iHit_F].adcY;

        tPair.stripX1 = primaryHits_F[iHit_F].stripX;
        tPair.stripY1 = primaryHits_F[iHit_F].stripY;
        tPair.X1 = X1;
        tPair.Y1 = Y1;
        tPair.Z1 = Z1;
	tPair.timeAvg=primaryHits_F[iHit_F].timeAvg;
        tPair.trackStartPosition=trackStart;
	tPair.flashTDiff=minTimeDifference;
	tPair.trackEndPosition=trackEnd;
        tracksPair_B.push_back(tPair);

      }
	}
 if ( (trackStartPositionZ<620 && trackEndPositionZ > 660 && trackStartPositionZ > 300)) {
      for (unsigned int iHit_B = 0; iHit_B < primaryHits_B.size(); iHit_B++) {
	if (!fMCCSwitch && moduletoCTB(primaryHits_B[iHit_B].moduleX, primaryHits_B[iHit_B].moduleY)!=pixel1) continue;
        double X1 = primaryHits_B[iHit_B].hitPositionX;

        double Y1 = primaryHits_B[iHit_B].hitPositionY;

        double Z1 = primaryHits_B[iHit_B].hitPositionZ;


 
	// Make metrics for a CRT pair to compare later
	TVector3 trackStart(trackStartPositionX, trackStartPositionY, trackStartPositionZ);
	TVector3 trackEnd(trackEndPositionX, trackEndPositionY, trackEndPositionZ);
	TVector3 v1(X1,Y1,Z1);
	TVector3 v2(trackStartPositionX, trackStartPositionY, trackStartPositionZ);

            TVector3 v4(trackStartPositionX,
                        trackStartPositionY,
                        trackStartPositionZ);
            TVector3 v5(trackEndPositionX,
                        trackEndPositionY,
                        trackEndPositionZ);
	TVector3 trackVector = (v5-v4).Unit();
	TVector3 hitVector=(v2-v1).Unit();




              double predictedHitPositionY1 = (v1.Z()-v5.Z())/(v4.Z()-v5.Z())*(v4.Y()-v5.Y())+v5.Y();


              double predictedHitPositionX1 = (v1.Z()-v5.Z())/(v4.Z()-v5.Z())*(v4.X()-v5.X())+v5.X();

	double dotProductCos=trackVector*hitVector;

        double deltaX1 = (predictedHitPositionX1-X1);

	double deltaX=(deltaX1);

        double deltaY1 = (predictedHitPositionY1-Y1);

	double deltaY=(deltaY1);
	double minTimeDifference=999999.99;
       for (unsigned int iFlash = 0; iFlash < opHitList.size(); iFlash++)
                {
			/*
                    if (flashList[iFlash].PE() < 100)
                        {
                            continue;
                        }
			*/
                    double hitTime = opHitList[iFlash]->PeakTime();
                    double timeDifference = primaryHits_B[iHit_B].timeAvg - hitTime*1000;

                    if(fabs(timeDifference) < fabs(minTimeDifference))
                        {
                            minTimeDifference = timeDifference;
                        }
                }

        tracksPair tPair;
        tPair.tempId = tempId;
        tPair.CRTTrackId = iHit_B;
        tPair.recoId = iRecoTrack;

	tPair.deltaX=deltaX;
        tPair.deltaY=deltaY;
        tPair.dotProductCos=dotProductCos;

        tPair.moduleX1 = primaryHits_B[iHit_B].moduleX;
        tPair.moduleY1 = primaryHits_B[iHit_B].moduleY;

        tPair.adcX1=primaryHits_B[iHit_B].adcX;
        tPair.adcY1=primaryHits_B[iHit_B].adcY;

        tPair.stripX1 = primaryHits_B[iHit_B].stripX;
        tPair.stripY1 = primaryHits_B[iHit_B].stripY;
        tPair.X1 = X1;
        tPair.Y1 = Y1;
        tPair.Z1 = Z1;
	tPair.timeAvg=primaryHits_B[iHit_B].timeAvg;
	tPair.flashTDiff=minTimeDifference;
        tPair.trackStartPosition=trackStart;
	tPair.trackEndPosition=trackEnd;
        tracksPair_B.push_back(tPair);

      }

      tempId++;
    } //iRecoTrack
    }

     //Sort pair by ascending order of absolute distance
    sort(tracksPair_F.begin(), tracksPair_F.end(), sortPair());
    sort(tracksPair_B.begin(), tracksPair_B.end(), sortPair());
    // Compare, sort, and eliminate CRT hits for just the best one
    // Compare, sort, and eliminate CRT hits for just the best one

    vector < tracksPair > allUniqueTracksPair;
    while (tracksPair_F.size()) {
      allUniqueTracksPair.push_back(tracksPair_F.front());
      tracksPair_F.erase(remove_if(tracksPair_F.begin(), tracksPair_F.end(), removePairIndex(tracksPair_F.front())),
        tracksPair_F.end());
    }

    while (tracksPair_B.size()) {
      allUniqueTracksPair.push_back(tracksPair_B.front());
      tracksPair_B.erase(remove_if(tracksPair_B.begin(), tracksPair_B.end(), removePairIndex(tracksPair_B.front())),
        tracksPair_B.end());
    }

	cout<<"Number of reco and CRT pairs: "<<allUniqueTracksPair.size()<<endl;
// For the best one, add the validation metrics to a tree
    if (allUniqueTracksPair.size() > 0) {
      for (unsigned int u = 0; u < allUniqueTracksPair.size(); u++) {


	deltaX=allUniqueTracksPair[u].deltaX;

	deltaY=allUniqueTracksPair[u].deltaY;
	opCRTTDiff=allUniqueTracksPair[u].flashTDiff;

	dotCos=fabs(allUniqueTracksPair[u].dotProductCos);
	trackX1=allUniqueTracksPair[u].trackStartPosition.X();
	trackY1=allUniqueTracksPair[u].trackStartPosition.Y();
	trackZ1=allUniqueTracksPair[u].trackStartPosition.Z();

	trackX2=allUniqueTracksPair[u].trackEndPosition.X();
	trackY2=allUniqueTracksPair[u].trackEndPosition.Y();
	trackZ2=allUniqueTracksPair[u].trackEndPosition.Z();

	moduleX=allUniqueTracksPair[u].moduleX1;
	moduleY=allUniqueTracksPair[u].moduleY1;

	adcX=allUniqueTracksPair[u].adcX1;
	adcY=allUniqueTracksPair[u].adcY1;

	CRTT0=allUniqueTracksPair[u].timeAvg;
	stripX=allUniqueTracksPair[u].stripX1;
	stripY=allUniqueTracksPair[u].stripY1;

	X_CRT=allUniqueTracksPair[u].X1;
	Y_CRT=allUniqueTracksPair[u].Y1;
	Z_CRT=allUniqueTracksPair[u].Z1;

       	
        if (fabs(allUniqueTracksPair[u].dotProductCos)>0.999 && fabs(allUniqueTracksPair[u].deltaY)<150 && fabs(fOpCRTTDiffCut)<200) {
	cout<<fabs(allUniqueTracksPair[u].dotProductCos)<<endl;

	fCRTTree->Fill();
	   

        }
      }
    }
  nEvents++;
 }


// Setup CRT 
void CRT::SingleCRTMatching::beginJob() {
	art::ServiceHandle<art::TFileService> fileServiceHandle;
       fCRTTree = fileServiceHandle->make<TTree>("Displacement", "event by event info");
	fCRTTree->Branch("nEvents", &nEvents, "fnEvents/I");


	fCRTTree->Branch("hdeltaX", &deltaX, "deltaX/D");
	fCRTTree->Branch("hdeltaY", &deltaY, "deltaY/D");
	fCRTTree->Branch("hdotProductCos", &dotCos, "dotCos/D");


	fCRTTree->Branch("hX_CRT", &X_CRT, "X_CRT/D");
	fCRTTree->Branch("hY_CRT", &Y_CRT, "Y_CRT/D");
	fCRTTree->Branch("hZ_CRT", &Z_CRT, "Z_CRT/D");

	fCRTTree->Branch("hCRTT0", &CRTT0, "CRTT0/D");


	fCRTTree->Branch("htrackStartX", &trackX1, "trackX1/D");
	fCRTTree->Branch("htrackStartY", &trackY1, "trackY1/D");
	fCRTTree->Branch("htrackStartZ", &trackZ1, "trackZ1/D");

	fCRTTree->Branch("htrackEndX", &trackX2, "trackX2/D");
	fCRTTree->Branch("htrackEndY", &trackY2, "trackY2/D");
	fCRTTree->Branch("htrackEndZ", &trackZ2, "trackZ2/D");

	fCRTTree->Branch("hmoduleX", &moduleX, "moduleX/I");
	fCRTTree->Branch("hmoduleY", &moduleY, "moduleY/I");

	fCRTTree->Branch("hstripX", &stripX, "stripX/I");
	fCRTTree->Branch("hstripY", &stripY, "stripY/I");

	fCRTTree->Branch("hadcX", &adcX, "adcX/I");
	fCRTTree->Branch("hadcY", &adcY, "adcY/I");

	fCRTTree->Branch("hopCRTTDiff", &opCRTTDiff, "opCRTTDiff/D");


}
// Endjob actions
void CRT::SingleCRTMatching::endJob() 
{



}














DEFINE_ART_MODULE(CRT::SingleCRTMatching)