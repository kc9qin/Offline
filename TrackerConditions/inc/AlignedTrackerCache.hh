#ifndef TrackerConditions_AlignedTrackerCache_hh
#define TrackerConditions_AlignedTrackerCache_hh

//
// hold a set of run-dependent conditions objects
// and update them when needed
//

#include "Offline/Mu2eInterfaces/inc/ProditionsCache.hh"
#include "Offline/DbService/inc/DbHandle.hh"
#include "Offline/DbTables/inc/TrkAlignElement.hh"
#include "Offline/DbTables/inc/TrkAlignStraw.hh"
#include "Offline/TrackerConditions/inc/AlignedTrackerMaker.hh"


namespace mu2e {
  class AlignedTrackerCache : public ProditionsCache {
    public:
      AlignedTrackerCache(AlignedTrackerConfig const& config):
        ProditionsCache(Tracker::cxname,config.verbose()),
        _useDb(config.useDb()),_maker(config) {}

      void initialize() {
        if(_useDb) {
          _tatr_p = std::make_unique<DbHandle<TrkAlignTracker>>();
          _tapl_p = std::make_unique<DbHandle<TrkAlignPlane>>();
          _tapa_p = std::make_unique<DbHandle<TrkAlignPanel>>();
          _tast_p = std::make_unique<DbHandle<TrkAlignStraw>>();
        }
      }

      set_t makeSet(art::EventID const& eid) {
        // get the tables up to date
        ProditionsEntity::set_t cids;
        if(_useDb) {
          _tatr_p->get(eid);
          _tapl_p->get(eid);
          _tapa_p->get(eid);
          _tast_p->get(eid);
          cids.insert(_tatr_p->cid());
          cids.insert(_tapl_p->cid());
          cids.insert(_tapa_p->cid());
          cids.insert(_tast_p->cid());
        }
        return cids;
      }

      DbIoV makeIov(art::EventID const& eid) {
        DbIoV iov;
        iov.setMax(); // start with full IOV range
        if(_useDb) {
          iov.overlap(_tatr_p->iov());
          iov.overlap(_tapl_p->iov());
          iov.overlap(_tapa_p->iov());
          iov.overlap(_tast_p->iov());
        }
        return iov;
      }

      ProditionsEntity::ptr makeEntity(art::EventID const& eid) {
        if(_useDb) {
          return _maker.fromDb( _tatr_p->getPtr(eid),
              _tapl_p->getPtr(eid),
              _tapa_p->getPtr(eid),
              _tast_p->getPtr(eid) );
        } else {
          return _maker.fromFcl();
        }
      }

    private:
      bool _useDb;
      AlignedTrackerMaker _maker;

      std::unique_ptr<DbHandle<TrkAlignTracker>> _tatr_p;
      std::unique_ptr<DbHandle<TrkAlignPlane>>   _tapl_p;
      std::unique_ptr<DbHandle<TrkAlignPanel>>   _tapa_p;
      std::unique_ptr<DbHandle<TrkAlignStraw>>   _tast_p;

  };
}

#endif
