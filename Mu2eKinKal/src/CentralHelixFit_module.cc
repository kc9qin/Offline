//
// KinKal fit module for CentralHelix
//
// Original author D. Brown (LBNL) 11/18/20
//
#include "KinKal/Trajectory/CentralHelix.hh"
#include "KinKal/General/Parameters.hh"
#include "KinKal/General/Vectors.hh"
using KTRAJ= KinKal::CentralHelix; // this must come before HelixFit
#include "Offline/Mu2eKinKal/inc/HelixFit_module.hh"
#include "Offline/TrkReco/inc/TrkUtilities.hh"
#include "Offline/GeneralUtilities/inc/Angles.hh"

namespace mu2e {
  using KinKal::DVEC;
  using KinKal::VEC3;
  class CentralHelixFit : public HelixFit {
    public:
      explicit CentralHelixFit(const Parameters& settings) :
        HelixFit(settings,TrkFitFlag::KKCentralHelix) {}
      // parameter-specific functions
      KTRAJ makeSeedTraj(HelixSeed const& hseed,TimeRange const& trange,VEC3 const& bnom, int charge) const override;
      bool goodFit(KKTRK const& ktrk) const override;
      virtual ~CentralHelixFit() {}
  };

  KTRAJ CentralHelixFit::makeSeedTraj(HelixSeed const& hseed,TimeRange const& trange,VEC3 const& bnom, int charge) const {
    double amsign = copysign(1.0,-charge*bnom.Z());
    DVEC pars;
    auto const& helix = hseed.helix();
    // radius and omega have inverse magnitude, omega is signed by the angular momentum
    pars[KTRAJ::omega_] = amsign/helix.radius();
    // phi0 is the azimuthal angle of the particle velocity vector at the point
    // of closest approach to the origin.  It's sign also depends on the angular
    // momentum.  To translate from the center, we need to reverse coordinates
    pars[KTRAJ::phi0_] = atan2(-amsign*helix.centerx(),amsign*helix.centery());
    // d0 describes the distance to the origin at closest approach.
    // It is signed by the particle angular momentum WRT the origin.
    // The Helix fit radial bias is anti-correlated with d0; correct for it here.
    pars[KTRAJ::d0_] =  amsign*(helix.rcent() - helix.radius());
    // the dip angle is measured WRT the perpendicular, signed by the z component of linear momentum
    pars[KTRAJ::tanDip_] = amsign*helix.lambda()/helix.radius();
    // must change conventions here: fz0 is the phi at z=0, z0 is defined at the point of closest approach
    // resolve the loop ambiguity such that the POCA is closest to z=0.
    double refphi = helix.fz0()+amsign*M_PI_2;
    double phi = pars[KTRAJ::phi0_];
    double dphi = Angles::deltaPhi(phi,refphi);
    // choose z0 (which loop) so that f=0 is as close to z=0 as possible
    pars[KTRAJ::z0_] = dphi*pars[KTRAJ::tanDip_]/pars[KTRAJ::omega_];
    pars[KTRAJ::t0_] = hseed.t0().t0();
    // create the initial trajectory
    KinKal::Parameters kkpars(pars,seedcov_);
    //  construct the seed trajectory
    //  CentralHelix has no bnom constructor FIXME
    return KTRAJ(kkpars, mass_, charge, bnom.Z(), trange);
  }

  bool CentralHelixFit::goodFit(KKTRK const& ktrk) const {
    // require physical consistency: fit can succeed but the result can have changed charge or helicity
    bool retval = ktrk.fitStatus().usable() &&
      ktrk.fitTraj().front().parameterSign()*ktrk.seedTraj().front().parameterSign() > 0 &&
      ktrk.fitTraj().front().omega()*ktrk.seedTraj().front().omega() > 0 &&
      ktrk.fitTraj().front().tanDip()*ktrk.seedTraj().front().tanDip() > 0;
    // also check that the fit is inside the physical detector volume.  Test where the StrawHits are
    if(retval){
      for(auto const& shptr : ktrk.strawHits()) {
        if(shptr->active() && !Mu2eKinKal::inDetector(ktrk.fitTraj().position3(shptr->time()))){
          retval = false;
          break;
        }
      }
    }
    // test that the spatial parameter covariances and values aren't crazy TODO
    return retval;
  }
}
DEFINE_ART_MODULE(mu2e::CentralHelixFit)
