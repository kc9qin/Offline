//
//  Merge HelixSeeds from different pattern recognition paths.  Modeled
//  on MergeHelixFinder.  P. Murat, D. Brown (LBNL)
//
#include "fhiclcpp/types/Atom.h"
#include "fhiclcpp/types/Sequence.h"
#include "canvas/Utilities/InputTag.h"
#include "canvas/Persistency/Common/Ptr.h"
#include "art/Framework/Core/EDProducer.h"
#include "art/Framework/Principal/Selector.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"

#include "Offline/GeometryService/inc/GeometryService.hh"
#include "Offline/GeometryService/inc/GeomHandle.hh"
#include "Offline/GeometryService/inc/DetectorSystem.hh"
#include "Offline/CalorimeterGeom/inc/DiskCalorimeter.hh"
#include "Offline/CalorimeterGeom/inc/Calorimeter.hh"
#include "Offline/CalPatRec/inc/CalTimePeakFinder_types.hh"

// mu2e data products
#include "Offline/RecoDataProducts/inc/HelixSeed.hh"
#include "Offline/RecoDataProducts/inc/TimeCluster.hh"

#include "Offline/RecoDataProducts/inc/CaloCluster.hh"

// utilities
#include "Offline/TrkReco/inc/TrkUtilities.hh"

#include "Offline/Mu2eUtilities/inc/LsqSums2.hh"
#include "Offline/Mu2eUtilities/inc/LsqSums4.hh"
#include "Offline/Mu2eUtilities/inc/polyAtan2.hh"
// C++
#include <vector>
#include <memory>
#include <iostream>
#include <forward_list>
#include <string>

using CLHEP::Hep3Vector;

namespace mu2e {
  class MergeHelices : public art::EDProducer {
  public:
    using Name=fhicl::Name;
    using Comment=fhicl::Comment;
    struct Config {
      fhicl::Atom<int> debug{ Name("debugLevel"),
	      Comment("Debug Level"), 0};
      fhicl::Atom<bool> selectbest{ Name("SelectBest"),
	      Comment("Select best overlapping helices for output"), true};
      fhicl::Atom<bool> usecalo{ Name("UseCalo"),
	      Comment("Use CaloCluster info in comparison"), true};
      fhicl::Atom<unsigned> minnover{ Name("MinNHitOverlap"),
	      Comment("Minimum number of common hits to consider helices to be 'the same'"), 10};
      fhicl::Atom<float> minoverfrac{ Name("MinHitOverlapFraction"),
	      Comment("Minimum fraction of common hits to consider helices to be 'the same'"), 0.5};
      fhicl::Sequence<std::string> BadHitFlags { Name("BadHitFlags"),
	      Comment("HelixHit flag bits to exclude from counting"),std::vector<std::string>{"Outlier"}}; 
      fhicl::Sequence<std::string> HelixFinders { Name("HelixFinders"),
	      Comment("HelixSeed producers to merge")};
    };
    using Parameters = art::EDProducer::Table<Config>;
    explicit MergeHelices(const Parameters& conf);
    void produce(art::Event& evt) override;
  private:
    enum HelixComp{unique=-1,first=0,second=1};
    int _debug;
    unsigned _minnover;
    float _minoverfrac;
    bool _selectbest, _usecalo;
    std::vector<std::string> _hfs;
    StrawHitFlag _badhit;
    // helper functions
    typedef std::vector<StrawHitIndex> SHIV;
    HelixComp compareHelices(art::Event const& evt,
	      HelixSeed const& h1, HelixSeed const& h2);
    void countHits(art::Event const& evt,
	      HelixSeed const& h1, HelixSeed const& h2,
	      unsigned& nh1, unsigned& nh2, unsigned& nover);
    unsigned countOverlaps(SHIV const& s1, SHIV const& s2);
    void findchisq(art::Event const& evt,
        HelixSeed const& h2, float& chixy, float& chizphi);
  };

  MergeHelices::MergeHelices(const Parameters& config) : 
    art::EDProducer{config},
    _debug(config().debug()),
    _minnover(config().minnover()),
    _minoverfrac(config().minoverfrac()),
    _selectbest(config().selectbest()),
    _usecalo(config().usecalo()),
    _hfs(config().HelixFinders()),
    _badhit(config().BadHitFlags())
    {
      consumesMany<HelixSeedCollection>    ();
      produces<HelixSeedCollection>    ();
      produces<TimeClusterCollection>    ();
    }

  void MergeHelices::produce(art::Event& event) {
    // create output
    std::unique_ptr<HelixSeedCollection> mhels(new HelixSeedCollection);
    std::unique_ptr<TimeClusterCollection> tcs(new TimeClusterCollection);
    // needed for creating Ptrs
    auto TimeClusterCollectionPID = event.getProductID<TimeClusterCollection>();
    auto TimeClusterCollectionGetter = event.productGetter(TimeClusterCollectionPID);
    // loop over helix products and flatten the helix collections into a single collection
    std::list<const HelixSeed*> hseeds;
    for(auto const& hf : _hfs) {
      art::InputTag hsct(hf);
      auto hsch = event.getValidHandle<HelixSeedCollection>(hsct);
      auto const& hsc = *hsch;
      for(auto const&  hs : hsc) {
	      hseeds.insert(hseeds.end(),&hs);
      }
    }
    // now loop over all combinations
    for(auto ihel = hseeds.begin(); ihel != hseeds.end();) {
      auto jhel = ihel; jhel++;
      while( jhel != hseeds.end()){
	      // compare the helices
       	auto hcomp = compareHelices(event, **ihel, **jhel);
	     if(hcomp == unique) {
	       // both helices are unique: simply advance the iterator to keep both
	       jhel++;
	     } else if(hcomp == first) {
	        // the first helix is 'better'; remove the second
	        jhel = hseeds.erase(jhel);
	     } else if(hcomp == second) {
	       // the second helix is better; remove the first and restart the loop
	       ihel = hseeds.erase(ihel);
	       break;
	     }
      }
      // only advance the outer loop if we exhausted the inner one
      if(jhel == hseeds.end())ihel++;
    }
    // now hseeds contains only pointers to unique and best helices: use them to 
    // create the output, which must be deep-copy, including the time cluster
    for(auto ihel = hseeds.begin(); ihel != hseeds.end(); ihel++){
      // copy the time cluster first
      tcs->push_back(*(*ihel)->timeCluster());
      // create a Ptr to the new TimeCluster
      auto tcptr = art::Ptr<TimeCluster>(TimeClusterCollectionPID,tcs->size()-1,TimeClusterCollectionGetter);
      // copy the Helix Seed and point its TimeCluster to the copy
      HelixSeed hs(**ihel);
      hs._timeCluster = tcptr;
      float chixy(0),chizphi(0);
      // Calculate the XY and ZPhi chisquared of the helix
      findchisq(event,**ihel,chixy,chizphi); 
      // Replace the helix seed chisquared with the new values   
      hs._helix._chi2dXY = chixy;
      hs._helix._chi2dZPhi = chizphi;
      hs._timeCluster = tcptr;
      mhels->push_back(hs);
    }
    event.put(std::move(mhels));
    event.put(std::move(tcs));
  }

  MergeHelices::HelixComp MergeHelices::compareHelices(art::Event const& evt,
      HelixSeed const& h1, HelixSeed const& h2) {
    HelixComp retval(unique);
    // count the StrawHit overlap between the helices
    unsigned nh1, nh2, nover;
    countHits(evt,h1,h2, nh1, nh2, nover);
    unsigned minh = std::min(nh1, nh2);
    float chih1xy(0),chih1zphi(0),chih2xy(0),chih2zphi(0);
    int deltanh = 5; 
    // Calculate the chi-sq of the helices
    findchisq(evt,h1,chih1xy,chih1zphi);
    findchisq(evt,h2,chih2xy,chih2zphi);
    if(nover >= _minnover && nover/float(minh) > _minoverfrac) {
      // overlapping helices: decide which is best
      // Pick the one with a CaloCluster first	    
      if(h1.caloCluster().isNonnull() && h2.caloCluster().isNull())
	      retval = first;
      else if( h2.caloCluster().isNonnull() && h1.caloCluster().isNull())
	      retval = second;
	    // then compare active StrawHit counts and if difference of the StrawHit counts greater than deltanh 
      else if((nh1 > nh2) && (nh1-nh2) > deltanh)
	      retval = first;
      else if((nh2 > nh1) && (nh2-nh1) > deltanh)
	      retval = second;
	    // finally compare chisquared: sum xy and fz 
      else if(chih1xy+chih1zphi  < chih2xy+chih2zphi)
	      retval = first;
      else
	      retval = second;
    }
    return retval;
  }

  void MergeHelices::countHits(art::Event const& evt,
      HelixSeed const& h1, HelixSeed const& h2,
      unsigned& nh1, unsigned& nh2, unsigned& nover) {
    nh1 = nh2 = nover = 0;
    SHIV shiv1, shiv2;
    for(size_t ihit=0;ihit < h1.hits().size(); ihit++){
      auto const& hh = h1.hits()[ihit];
      if(!hh.flag().hasAnyProperty(_badhit))
	    h1.hits().fillStrawHitIndices(evt,ihit,shiv1);
    }
    for(size_t ihit=0;ihit < h2.hits().size(); ihit++){
      auto const& hh = h2.hits()[ihit];
      if(!hh.flag().hasAnyProperty(_badhit))
	   h2.hits().fillStrawHitIndices(evt,ihit,shiv2);
    }
    nh1 = shiv1.size();
    nh2 = shiv2.size();
    nover = countOverlaps(shiv1,shiv2);
  }
	
  // The chisquared calculation method used here is 
  // taken from the LsqSums approach followed in CalPatRec
  void MergeHelices::findchisq(art::Event const& evt,
      HelixSeed const& h1,float& chixy, float& chizphi) {
    ::LsqSums4 sxy;
    ::LsqSums4 szphi;
    for(size_t ihit=0;ihit < h1.hits().size(); ihit++) {
      auto const& hh = h1.hits()[ihit];
      float transErr = 5./sqrt(12.);
      if(!hh.flag().hasAnyProperty(_badhit)){
	      // Calculate the transverse and z-phi weights of the hits
        if(hh.nStrawHits() > 1) transErr *= 1.5;
	      float transErr2 = transErr*transErr;
        float dx  = hh.pos().x() - h1.helix().center().x();
        float dy  = hh.pos().y() - h1.helix().center().y();
        float dxn = dx*hh._sdir.x()+dy*hh._sdir.y();
        float costh2 = dxn*dxn/(dx*dx+dy*dy);
        float sinth2 = 1-costh2;
        float e2xy     = hh.wireErr2()*sinth2+transErr2*costh2;
        float wtxy     = 1./e2;
        float e2zphi     = hh.wireErr2()*costh2+hh.transErr2()*sinth2;        
        float wtzphi     = h1.helix().radius()*h1.helix().radius()/e2zphi;
        wtxy      *=1.1;
        wtzphi  *=0.75;        
        sxy.addPoint(hh.pos().x(),hh.pos().y(),wtxy);
        szphi.addPoint(hh.pos().z(),hh.helixPhi(),wtzphi);
      }
    }
    chixy = sxy.chi2DofCircle();
    chizphi = szphi.chi2DofLine();
  }
  
  unsigned MergeHelices::countOverlaps(SHIV const& shiv1, SHIV const& shiv2) {
    unsigned retval(0);
    for(auto shi1 : shiv1)
      for(auto shi2 : shiv2)
	      if(shi1 == shi2)retval++;
    return retval;
  }
}
DEFINE_ART_MODULE(mu2e::MergeHelices)

