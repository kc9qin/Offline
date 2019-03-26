////////////////////////////////////////////////////////////////////////
// Class:       PrescaleEvent
// Module Type: filter
// File:        PrescaleEvent_module.cc
//
// Generated at Tue Jan  6 11:52:08 2015 by John Freeman using artmod
// from cetpkgsupport v1_07_01.
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDFilter.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "canvas/Utilities/InputTag.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "RecoDataProducts/inc/TriggerInfo.hh"

#include <memory>
#include <iostream>

namespace mu2e
{

  class PrescaleEvent : public art::EDFilter {

    public:

      explicit PrescaleEvent(fhicl::ParameterSet const & p);

      // The destructor generated by the compiler is fine for classes
      // without bare pointers or other resource use.
      // Plugins should not be copied or assigned.

      PrescaleEvent(PrescaleEvent const &) = delete;
      PrescaleEvent(PrescaleEvent &&) = delete;
      PrescaleEvent & operator = (PrescaleEvent const &) = delete;
      PrescaleEvent & operator = (PrescaleEvent &&) = delete;

      bool filter(art::Event & e) override;
      virtual bool endRun(art::Run& run ) override;

    private:

    uint32_t     nPrescale_;
    bool         useFilteredEvts_;
    int          _debug;
    TriggerFlag  _trigFlag;
    std::string  _trigPath;
    unsigned     _nevt, _npass;

  };

  PrescaleEvent::PrescaleEvent(fhicl::ParameterSet const & p)
    : nPrescale_      (p.get<uint32_t>("nPrescale")), 
      useFilteredEvts_(p.get<bool>    ("useFilteredEvents",false)), 
      _debug          (p.get<int>     ("debugLevel",0)), 
      _trigFlag       (p.get<std::vector<std::string> >("triggerFlag")),
      _trigPath       (p.get<std::string>("triggerPath")),
      _nevt(0), _npass(0)
  {
    produces<TriggerInfo>();
  }

  inline bool PrescaleEvent::filter(art::Event & e)
  {
    std::unique_ptr<TriggerInfo> trigInfo(new TriggerInfo);
    ++_nevt;
    bool retval(false);
    bool condition = e.event() % nPrescale_ == 0;
    if (useFilteredEvts_) condition = _nevt % nPrescale_ == 0;

    if(condition) {
      ++_npass;
      //      trigInfo->_triggerBits.merge(TriggerFlag::prescaleRandom);
      trigInfo->_triggerBits.merge(_trigFlag);
      trigInfo->_triggerPath = _trigPath;
      retval = true;
    }
    e.put(std::move(trigInfo));
    return retval;
  }

  bool PrescaleEvent::endRun( art::Run& run ) {
    if(_debug > 0){
      std::cout << *currentContext()->moduleLabel() << " passed " << _npass << " events out of " << _nevt << " for a ratio of " << float(_npass)/float(_nevt) << std::endl;
    }
    return true;
  }

}
using mu2e::PrescaleEvent;
DEFINE_ART_MODULE(PrescaleEvent)
