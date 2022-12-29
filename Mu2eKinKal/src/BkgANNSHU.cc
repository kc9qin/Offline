#include "Offline/Mu2eKinKal/inc/BkgANNSHU.hh"
#include "Offline/RecoDataProducts/inc/ComboHit.hh"
#include "Offline/Mu2eKinKal/inc/StrawHitUpdaters.hh"
#include "Offline/ConfigTools/inc/ConfigFileLookupPolicy.hh"
#include "Offline/Mu2eKinKal/inc/TrainBkg.hxx"
#include <cmath>
#include <array>

namespace mu2e {
  using KinKal::ClosestApproachData;
  using KinKal::VEC3;
  BkgANNSHU::BkgANNSHU(Config const& config) {
    ConfigFileLookupPolicy configFile;
    auto mvaWgtsFile = configFile(std::get<0>(config));
    mva_ = std::make_shared<TMVA_SOFIE_TrainBkg::Session>(mvaWgtsFile);
    mvacut_ = std::get<1>(config);
    std::string freeze = std::get<2>(config);
    diag_ = std::get<3>(config);
    freeze_ = WHSMask(freeze);
    if(diag_ > 0)std::cout << "BkgANNSHU weights " << std::get<0>(config) << " cut " << mvacut_ << " freeze " << freeze_ << std::endl;
  }

  WireHitState BkgANNSHU::wireHitState(WireHitState const& input, ClosestApproachData const& tpdata, DriftInfo const& dinfo, ComboHit const& chit) const {
    WireHitState whstate = input;
    if(input.updateable(StrawHitUpdaters::BkgANN)){
      std::array<float,8> pars;
      // this order is given by the training
      pars[0] = fabs(tpdata.doca());
      pars[1] = dinfo.driftDistance_;
      pars[2] = tpdata.docaVar();
      pars[3] = chit.driftTime();
      pars[4] = chit.energyDep();
      pars[5] = chit.wireDist();
      // compare the delta-t based U position with the fit U position; requires relative end
      double endsign = 2.0*(chit.driftEnd()-0.5);
      double upos = -endsign*tpdata.sensorDirection().Dot(tpdata.sensorPoca().Vect() - chit.centerPos());
      pars[6] = upos;
      pars[7] = tpdata.particlePoca().Vect().Rho();
      auto mvaout = mva_->infer(pars.data());
      if(diag_ > 2){
        whstate.algo_  = StrawHitUpdaters::BkgANN;
        whstate.quality_ = mvaout[0];
      }
      if(mvaout[0] < mvacut_){
        whstate.algo_  = StrawHitUpdaters::BkgANN;
        whstate.state_ = WireHitState::inactive;
        whstate.frozen_ = whstate.isIn(freeze_);
        whstate.quality_ = mvaout[0];
      }
    }
    return whstate;
  }
  std::string const& BkgANNSHU::configDescription() {
    static std::string descrip( "Weight file, ANN cut, states to freeze, diag level");
    return descrip;
  }
}
