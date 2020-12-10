#ifndef DbTables_TrkAlignStraw_hh
#define DbTables_TrkAlignStraw_hh

#include <string>
#include <iomanip>
#include <sstream>
#include <map>
#include "DbTables/inc/DbTable.hh"
#include "DbTables/inc/TrkStrawEndAlign.hh"
#include "CLHEP/Vector/ThreeVector.h"

namespace mu2e {

  class TrkAlignStraw : public DbTable {
  public:
    using xyzVec = CLHEP::Hep3Vector; // switch to XYZVec TODO

    typedef std::shared_ptr<TrkAlignStraw> ptr_t;
    typedef std::shared_ptr<const TrkAlignStraw> cptr_t;

    TrkAlignStraw():DbTable("TrkAlignStraw","trk.alignstraw",
	"index,StrawId,wire_cal_dV,wire_cal_dW,wire_hv_dV,wire_hv_dW,straw_cal_dV,straw_cal_dW,straw_hv_dV,straw_hv_dW") {} // this last should come from the Row class FIXME!
    const TrkStrawEndAlign& rowAt(const std::size_t index) const { return _rows.at(index);}
    std::vector<TrkStrawEndAlign> const& rows() const {return _rows;}
    size_t nrow() const override { return _rows.size(); };
    size_t nrowFix() const override { return StrawId::_nustraws; };
    size_t size() const override { return _csv.capacity() + nrow()*sizeof(TrkStrawEndAlign); };

    void addRow(const std::vector<std::string>& columns) override {
      _rows.emplace_back(
	  std::stoi(columns[0]),
	  StrawId(columns[1]),
	  std::stof(columns[2]),
	  std::stof(columns[3]),
	  std::stof(columns[4]),
	  std::stof(columns[5]),
	  std::stof(columns[6]),
	  std::stof(columns[7]),
	  std::stof(columns[8]),
	  std::stof(columns[9]) );
    }

    void rowToCsv(std::ostringstream& sstream, std::size_t irow) const override {
      TrkStrawEndAlign const& r = _rows.at(irow);
      sstream << r._index <<",";
      sstream << r.id().plane() << "_" << r.id().panel() << "_" << r.id().straw() << ",";
      sstream << std::fixed << std::setprecision(4); 
      sstream << r._wire_cal_dV <<",";
      sstream << r._wire_cal_dW <<",";
      sstream << r._wire_hv_dV <<",";
      sstream << r._wire_hv_dW <<",";
      sstream << r._straw_cal_dV <<",";
      sstream << r._straw_cal_dW <<",";
      sstream << r._straw_hv_dV <<",";
      sstream << r._straw_hv_dW;
    }

    virtual void clear() { _csv.clear(); _rows.clear(); }

  private:
    std::vector<TrkStrawEndAlign> _rows;
  };
  
};
#endif