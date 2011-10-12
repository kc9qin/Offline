//
//
// An enum-matched-to-names class for the names of the StepPointMC
// collections produced inside G4_module.
//
// $Id: StepInstanceName.cc,v 1.1 2011/10/12 20:00:21 kutschke Exp $
// $Author: kutschke $
// $Date: 2011/10/12 20:00:21 $
//
// Contact person Rob Kutschke
//

#include "MCDataProducts/inc/StepInstanceName.hh"

#include <boost/static_assert.hpp>

#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

using namespace std;

namespace mu2e {

  void StepInstanceName::printAll( std::ostream& ost){
    ost << "List of names of instances for StepPointMCCollections Id codes: "
        << endl;
    for ( int i=0; i<lastEnum; ++i){
      StepInstanceName id(i);
      ost << setw(2) << i << " " << name(id.id()) << std::endl;
    }
  }

  std::string const& StepInstanceName::name( enum_type id){
    return names().at(id);
  }

  std::vector<std::string> const& StepInstanceName::names(){

    static vector<string> nam;

    if ( nam.empty() ){
      const char* tmp[] = { STEPINSTANCENAME_NAMES };
      BOOST_STATIC_ASSERT(sizeof(tmp)/sizeof(char*) == lastEnum);
      nam.insert( nam.begin(), tmp, tmp+lastEnum);
    }

    return nam;
  }

  bool StepInstanceName::isValidorThrow( enum_type id){
    if ( !isValid(id) ){
      ostringstream os;
      os << "Invalid StepInstanceName::enum_type : "
         << id;
      throw std::out_of_range( os.str() );
    }
    return true;
  }

  StepInstanceName StepInstanceName::findByName( std::string const& name, bool throwIfUndefined ){
    std::vector<string> const& nam(names());

    // Size must be at least 2 (for unknown and lastEnum).
    for ( size_t i=0; i<nam.size(); ++i ){
      if ( nam[i] == name ){
        if ( i == unknown && throwIfUndefined ){
          throw std::out_of_range( "StepInstanceName the value \"unknown\" is not permitted in this context. " );
        }
        return StepInstanceName(i);
      }
    }

    if ( throwIfUndefined ) {
      ostringstream os;
      os << "StepInstanceName unrecognized name: "
         << name;
      throw std::out_of_range( os.str() );
    }

    return StepInstanceName(unknown);
  } // end StepInstanceName::findByName

} // end namespace mu2e
