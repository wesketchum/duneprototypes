////////////////////////////////////////////////////////////////////////
// Class:       HadCal
// Module Type: analyzer
// File:        HadCal_module.cc
//
// Generated at Wed Feb  8 09:38:58 2017 by Dorota Stefan using artmod
// from cetpkgsupport v1_11_00.
////////////////////////////////////////////////////////////////////////

#include "lardataobj/Simulation/SimChannel.h"
#include "larsim/Simulation/LArG4Parameters.h"
#include "larsim/Simulation/LArVoxelData.h"
#include "larsim/Simulation/LArVoxelList.h"
#include "larsim/Simulation/SimListUtils.h"
#include "larcore/Geometry/Geometry.h"
#include "larcore/Geometry/GeometryCore.h"
#include "lardataobj/RecoBase/Hit.h"
#include "lardataobj/RecoBase/Cluster.h"
#include "lardataobj/RecoBase/Track.h"
#include "lardataobj/AnalysisBase/Calorimetry.h"
#include "larreco/Calorimetry/CalorimetryAlg.h"
#include "lardataobj/RecoBase/TrackHitMeta.h"
#include "nusimdata/SimulationBase/MCParticle.h"
#include "nusimdata/SimulationBase/MCTruth.h"
#include "larcoreobj/SimpleTypesAndConstants/PhysicalConstants.h"
#include "lardata/Utilities/DatabaseUtil.h"

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "canvas/Persistency/Common/FindManyP.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art/Framework/Services/Optional/TFileService.h"
#include "canvas/Utilities/InputTag.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include "lardata/ArtDataHelper/MVAReader.h"

#include "TH1.h"
#include "TTree.h"
#include "TLorentzVector.h"
#include "TVector3.h"

#define MVA_LENGTH 4

namespace proto
{
	class HadCal;
}

class proto::HadCal : public art::EDAnalyzer {
public:
  explicit HadCal(fhicl::ParameterSet const & p);
  // The destructor generated by the compiler is fine for classes
  // without bare pointers or other resource use.

  // Plugins should not be copied or assigned.
  HadCal(HadCal const &) = delete;
  HadCal(HadCal &&) = delete;
  HadCal & operator = (HadCal const &) = delete;
  HadCal & operator = (HadCal &&) = delete;

  // Required functions.
  void analyze(art::Event const & e) override;
  
  void beginJob() override;

	void reconfigure(fhicl::ParameterSet const& p) override;

private:

  // Declare member data here.
  double GetEdepHitsMeV(const std::vector< recob::Hit > & hits) const;
  double GetEhitMeV(const recob::Hit & hit) const;
  double GetEkinMeV(const std::vector < art::Ptr< recob::Hit > > & hits, const std::vector < recob::TrackHitMeta const* > & data);
  double GetEdepEM_MC(art::Event const & e) const;
  double GetEdepHADh_MC(art::Event const & e, const std::vector< recob::Hit > & hits) const;
  void ResetVars();
  
  geo::GeometryCore const * fGeometry;
  
  TTree *fTree;
  TTree *fTreeEntries;
  int fRun;
  int fEvent;
  int fBestView;
  int fNumberOfTracks;
  double fEdepEM_MC;
  double fEdepHADh_MC;
  double fEMEnSum;
  double fHadEnSum;
  double fHadDepSum;
  double fTotEdepSum;
  double fdQdx;
  double fdEdx;
  double fdQ;
  double fdx;
  double fEdepSum;
  double fEdepAllhits;
  double fTotEnkinSum;
  double fElectronsToGeV;
  double fResRange;
  double fEnGen;
  double fEkGen;
  double fT0;
  
  // Module labels to get data products
  //anab::Calorimetry fCalorimetry;
  art::InputTag fNNetModuleLabel;
  std::string fHitModuleLabel;
  std::string fClusterModuleLabel;
  std::string fTrackModuleLabel;
  std::string fCalorimetryModuleLabel;
  std::string fSimulationLabel;
  calo::CalorimetryAlg fCalorimetryAlg;
  
  std::unordered_map< int, const simb::MCParticle* > fParticleMap;
};


proto::HadCal::HadCal(fhicl::ParameterSet const & p)
  :
  EDAnalyzer(p),
  fCalorimetryAlg(p.get<fhicl::ParameterSet>("CalorimetryAlg"))
 // More initializers here.
{
	reconfigure(p);
	// get a pointer to the geometry service provider
	fGeometry = &*(art::ServiceHandle<geo::Geometry>());
}

void proto::HadCal::beginJob()
{
	art::ServiceHandle<sim::LArG4Parameters> larParameters;
  fElectronsToGeV = 1./larParameters->GeVToElectrons();

	// access art's TFileService, which will handle creating and writing hists
	art::ServiceHandle<art::TFileService> tfs;	
	
	fTree = tfs->make<TTree>("calibration","calibration tree");
	fTree->Branch("fRun", &fRun, "fRun/I");
	fTree->Branch("fEvent", &fEvent, "fEvent/I");
	fTree->Branch("fEnGen", &fEnGen, "fEnGen/D");
	fTree->Branch("fEkGen", &fEkGen, "fEkGeb/D");
	fTree->Branch("fEdepEM_MC", &fEdepEM_MC, "fEdepEM_MC/D");
	fTree->Branch("fEdepHADh_MC", &fEdepHADh_MC, "fEdepHADh_MC/D");
	fTree->Branch("fNumberOfTracks", &fNumberOfTracks, "fNumberOfTracks/I");
	fTree->Branch("fHadEnSum", &fHadEnSum, "fHadEnSum/D");
	fTree->Branch("fHadDepSum", &fHadDepSum, "fHadDepSum/D");
	fTree->Branch("fEMEnSum", &fEMEnSum, "fEMEnSum/D");
	fTree->Branch("fTotEdepSum", &fTotEdepSum, "fTotEdepSum/D");
	fTree->Branch("fTotEnkinSum", &fTotEnkinSum, "fTotEnkinSum/D");
	fTree->Branch("fEdepSum", &fEdepSum, "fEdepSum/D");
	fTree->Branch("fEdepAllhits", &fEdepAllhits, "fEdepAllhits/D");
	fTree->Branch("fT0", &fT0, "fT0/D");
	
	fTreeEntries = tfs->make<TTree>("entries","entries tree");
	fTreeEntries->Branch("fdQdx", &fdQdx, "fdQdx/D");
	fTreeEntries->Branch("fdEdx", &fdEdx, "fdEdx/D");
	fTreeEntries->Branch("fdQ", &fdQ, "fdQ/D");
	fTreeEntries->Branch("fdx", &fdx, "fdx/D");
	
}

void proto::HadCal::reconfigure(fhicl::ParameterSet const & p)
{
	fHitModuleLabel = p.get< std::string >("HitModuleLabel");
	fClusterModuleLabel = p.get< std::string >("ClusterModuleLabel");
	fTrackModuleLabel = p.get< std::string >("TrackModuleLabel");
	fCalorimetryModuleLabel = p.get< std::string >("CalorimetryModuleLabel");
	fSimulationLabel = p.get< std::string >("SimulationLabel");
	fNNetModuleLabel = p.get< std::string >("NNetModuleLabel");
}

void proto::HadCal::analyze(art::Event const & e)
{
  // Implementation of required member function here.
  ResetVars();
  
  fRun = e.run();
  fEvent = e.id().event();
  
 	// MC particle list
	auto particleHandle = e.getValidHandle< std::vector<simb::MCParticle> >(fSimulationLabel);	
	bool flag = true;
	
	for (auto const& p : *particleHandle)
	{
		fParticleMap[p.TrackId()] = &p;
		if ((p.Process() == "primary") && flag)
		{
			fEnGen = p.P();
			fEkGen = (std::sqrt(p.P()*p.P() + p.Mass()*p.Mass()) - p.Mass()) * 1000; // MeV
			fT0 = p.T();
			flag = false;	
		}
	}
	
	fEdepEM_MC = GetEdepEM_MC(e);
	
	// hits
	const auto& hitListHandle = *e.getValidHandle< std::vector<recob::Hit> >(fHitModuleLabel);
	fEdepAllhits = GetEdepHitsMeV(hitListHandle); 
	
	// MC associated with a hit
	fEdepHADh_MC = GetEdepHADh_MC(e, hitListHandle);
  
  // clusters
  auto cluHandle = e.getValidHandle< std::vector<recob::Cluster> >(fClusterModuleLabel);
  art::FindManyP< recob::Hit > hitsFromCluster(cluHandle, e, fClusterModuleLabel);
  
  std::unordered_map<int, bool> hitIDE;
  for (size_t c = 0; c < cluHandle->size(); ++c)
  {
  	for (size_t h = 0; h < hitsFromCluster.at(c).size(); ++h)
  	{
  		if (hitsFromCluster.at(c)[h]->View() == fBestView)
  		{
  			hitIDE[hitsFromCluster.at(c)[h].key()] = false;
  		}	
  	}
  }  
 
  // output from cnn's
  auto trkResults = anab::MVAReader< recob::Track, MVA_LENGTH>::create(e, fNNetModuleLabel);  

	fHadEnSum = 0.0;
	fHadDepSum = 0.0;
	fEMEnSum = 0.0;

  if (trkResults)
  {
  	fNumberOfTracks = 0;
  
  	// use handle and input tag of reco objects associated to cnn output
  	const art::FindManyP< anab::Calorimetry > calFromTracks(trkResults->dataHandle(), e, fCalorimetryModuleLabel);	
  	const art::FindManyP< recob::Hit > hitsFromTracks(trkResults->dataHandle(), e, fTrackModuleLabel);
  	const art::FindManyP<recob::Hit, recob::TrackHitMeta> fmthm(trkResults->dataHandle(), e, fTrackModuleLabel);
  	
  	if (fmthm.isValid())
  	{
  		// loop over tracks
  		for (size_t t = 0; t < trkResults->size(); ++t)
  		{	
  			auto vhit = fmthm.at(t);
  			auto vmeta = fmthm.data(t);

  	  	// mark that hits have been used 
  			for (size_t h = 0; h < hitsFromTracks.at(t).size(); ++h)
  			{
  				if (hitsFromTracks.at(t)[h]->View() == fBestView)
  				{
  					hitIDE[hitsFromTracks.at(t)[h].key()] = true;
  				}
  			}	
  	
  			int nplanes = calFromTracks.at(t).size(); 	
  			std::array< float, MVA_LENGTH > cnn_out = trkResults->getOutput(t); 
  		
  			// condition for tracks
  			if ( (cnn_out[1] / (cnn_out[0] + cnn_out[1])) < 0.63)
  			{
					fNumberOfTracks++;
				
					// for now it work for 3 planes and only collection view
  				if (nplanes == 3)
  				{
  					fHadEnSum += GetEkinMeV(vhit, vmeta);
  					for (size_t h = 0; h < hitsFromTracks.at(t).size(); ++h)
  					{
  						if (hitsFromTracks.at(t)[h]->View() == fBestView)
  						{
  							fHadDepSum += GetEhitMeV(*hitsFromTracks.at(t)[h]);
  						}
  					}
  				}
  			}
				else // ... and condition for em showers
				{
					for (size_t h = 0; h < hitsFromTracks.at(t).size(); ++h)
  				{
  					// kinetic energy without correction for recombination, 
  					// only one view is considered 
  					if (hitsFromTracks.at(t)[h]->View() == fBestView)
  					{
  						fEMEnSum += GetEhitMeV(*hitsFromTracks.at(t)[h]);  
  					}
  				}
				}
 			}
 		}
 }
 // output from cnn's
 auto cluResults = anab::MVAReader< recob::Cluster, MVA_LENGTH>::create(e, fNNetModuleLabel);
 
 if (cluResults)
 {
 		const art::FindManyP< recob::Hit > hitsFromClusters(cluResults->dataHandle(), e, fHitModuleLabel);
 		
 		// loop over clusters
		for (size_t c = 0; c < cluHandle->size(); ++c)
  	{
  		if ((*cluHandle)[c].View() == fBestView)
  		{ 			
  			// to think about: condition for EM. 
  			// now: all unused clusters are classified as EM showers
  			for (size_t h = 0; h < hitsFromCluster.at(c).size(); ++h)
  			{  				
  				if (hitIDE[hitsFromCluster.at(c)[h].key()] == false)
  				{
  					hitIDE[hitsFromCluster.at(c)[h].key()] = true;
  					fEMEnSum += GetEhitMeV(*hitsFromCluster.at(c)[h]);
  				}
  				
  				fEdepSum += GetEhitMeV(*hitsFromCluster.at(c)[h]);  
  			}
  		}
  	}
 }
 
 	fTotEnkinSum = (fEMEnSum / 0.55) + fHadEnSum;
 	fTotEdepSum = (fEMEnSum / 0.55) + (fHadDepSum / 0.33);
  fTree->Fill();	
}

// kinetic energy of hadronic part: correction for recombination applied according to Birks/Box model

/******************************************************************************/

double proto::HadCal::GetEkinMeV(const std::vector < art::Ptr< recob::Hit > > & hits, const std::vector < recob::TrackHitMeta const* > & data) 
{
	double ekin = 0.0;
	
	if (!hits.size()) return ekin;
	

	for (size_t h = 0; h < hits.size(); ++h)
	{
		double dqadc = hits[h]->Integral();
		if (!std::isnormal(dqadc) || (dqadc < 0)) dqadc = 0.0;
		
		unsigned short plane = hits[h]->WireID().Plane;
		unsigned short time = hits[h]->PeakTime();
		double t0 = 0;
		
		fdQdx = 0.0;
		fdQ = dqadc;
  	fdx = data[h]->Dx();
  	if ((fdx > 0) && (fdQ > 0))
  	{
  		fdQdx = fdQ/fdx;
  		fdEdx = fCalorimetryAlg.dEdx_AREA(fdQdx, time, plane, t0);
  		if (fdEdx > 35) fdEdx = 35;
  		
  		ekin += ( fdEdx * fdx );
  	}
  	else if ((fdx == 0) && (fdQ > 0))
  	{
  		
  	}
  	
  	fTreeEntries->Fill();
	}
	
	return ekin;
}

/******************************************************************************/

double proto::HadCal::GetEdepHitsMeV(const std::vector< recob::Hit > & hits) const
{
	if (!hits.size()) return 0.0;

	double dqsum = 0.0;
	for (size_t h = 0; h < hits.size(); ++h)
	{
		double dqadc = hits[h].Integral();
		if (!std::isnormal(dqadc) || (dqadc < 0)) continue;
	
		unsigned short plane = hits[h].WireID().Plane;
		if (plane == fBestView)
		{
			double tdrift = hits[h].PeakTime();
			double dqel = fCalorimetryAlg.ElectronsFromADCArea(dqadc, plane);
		
			double correllifetime = fCalorimetryAlg.LifetimeCorrection(tdrift, fT0);
			double dq = dqel * correllifetime * fElectronsToGeV * 1000;

			if (!std::isnormal(dq) || (dq < 0)) continue;

			dqsum += dq;
		} 
	}

	return dqsum; 
}

/******************************************************************************/

double proto::HadCal::GetEhitMeV(const recob::Hit & hit) const
{
	double dqadc = hit.Integral();
	if (!std::isnormal(dqadc) || (dqadc < 0)) dqadc = 0.0;

	unsigned short plane = hit.WireID().Plane;
	double tdrift = hit.PeakTime();
	double dqel = fCalorimetryAlg.ElectronsFromADCArea(dqadc, plane);

	double correllifetime = fCalorimetryAlg.LifetimeCorrection(tdrift, fT0);
	double dq = dqel * correllifetime * fElectronsToGeV * 1000;
	if (!std::isnormal(dq) || (dq < 0)) dq = 0.0;	

	return dq; 
}

/******************************************************************************/

double proto::HadCal::GetEdepEM_MC(art::Event const & e) const
{
	double enEM = 0.0;
	
	art::Handle< std::vector<sim::SimChannel> > simchannelHandle;
	if (e.getByLabel(fSimulationLabel, simchannelHandle))
	{
			for ( auto const& channel : (*simchannelHandle) )
			{
				if (fGeometry->View(channel.Channel()) == fBestView)
				{ 
					// for every time slice in this channel:
					auto const& timeSlices = channel.TDCIDEMap();
					for ( auto const& timeSlice : timeSlices )
					{
						// loop over the energy deposits.
						auto const& energyDeposits = timeSlice.second;
		
						for ( auto const& energyDeposit : energyDeposits )
						{
							double energy = energyDeposit.energy;
							int trackID = energyDeposit.trackID;
							
							if (trackID < 0)
							{
								enEM += energy;
							}
							else if (trackID > 0)
							{
								auto search = fParticleMap.find(trackID);
								bool found = true;
								if (search == fParticleMap.end())
								{
									mf::LogWarning("TrainingDataAlg") << "PARTICLE NOT FOUND";
									found = false;
								}
								
								int pdg = 0;
								if (found)
								{
									const simb::MCParticle& particle = *((*search).second);
                  if (!pdg) pdg = particle.PdgCode(); // not EM activity so read what PDG it is
								}
								
								if ((pdg == 11) || (pdg == -11) || (pdg == 22)) enEM += energy;
							}
							
						}
					}
				}
			}
	}
	return enEM;
}

/******************************************************************************/

double proto::HadCal::GetEdepHADh_MC(art::Event const & e, const std::vector< recob::Hit > & hits) const
{
	double tothadhit = 0.0;
	
	auto simChannelHandle = e.getValidHandle< std::vector<sim::SimChannel> >(fSimulationLabel);
	const std::vector< sim::SimChannel > & channels = *simChannelHandle;
	
	for (auto const & hit: hits)
	{
		// the channel associated with this hit.
		auto hitChannelNumber = hit.Channel();

		double hitEn = 0.0; double hitEnTrk = 0;
       
		for (auto const & channel : channels)
		{
			if (channel.Channel() != hitChannelNumber) continue;
			if (fGeometry->View(channel.Channel()) != fBestView) continue;

			// for every time slice in this channel:
			auto const& timeSlices = channel.TDCIDEMap();
			for (auto const& timeSlice : timeSlices)
			{
				int time = timeSlice.first;
				if (std::abs(hit.TimeDistanceAsRMS(time)) < 1.0)
				{
					// loop over the energy deposits.
					auto const & energyDeposits = timeSlice.second;
		
					for (auto const & energyDeposit : energyDeposits)
					{
						int trackID = energyDeposit.trackID;

						double energy = energyDeposit.numElectrons * fElectronsToGeV * 1000;
						hitEn += energy;

						if (trackID < 0) { } // EM activity
						else if (trackID > 0)
						{
							auto search = fParticleMap.find(trackID);
							if (search != fParticleMap.end())
							{
								const simb::MCParticle & particle = *((*search).second);
								int pdg = particle.PdgCode(); // not EM activity so read what PDG it is

							  if ((pdg == 11) || (pdg == -11) || (pdg == 22)) {}
							  else {hitEnTrk += energy;}
              }
							else { mf::LogWarning("TrainingDataAlg") << "PARTICLE NOT FOUND"; }
						}
					}
				}
			}
		}
		
		double ratio = 0.0;
		if (hitEn > 0) 
		{
			ratio = hitEnTrk / hitEn;
		}
		double hadhit = GetEhitMeV(hit) * ratio;
		tothadhit += hadhit;
	}
    
  return tothadhit;
}

/******************************************************************************/

void proto::HadCal::ResetVars()
{
	fRun = 0;
	fEvent = 0;
	fBestView = 2;
	fNumberOfTracks = 0;
	fEdepEM_MC = 0.0;
	fEdepHADh_MC = 0.0;
	fEnGen = 0.0;
	fEkGen = 0.0;
  fHadEnSum = 0.0;
  fEdepSum = 0.0;
  fEdepAllhits = 0.0;
	fEMEnSum = 0.0;
	fTotEdepSum = 0.0;
	fTotEnkinSum = 0.0;
	fdQdx = 0.0;
	fdEdx = 0.0;
	fdQ = 0.0;
	fdx = 0.0;
	fResRange = 0.0;
	fT0 = 0.0;
	fParticleMap.clear();
}

DEFINE_ART_MODULE(proto::HadCal)
