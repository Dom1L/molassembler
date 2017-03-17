#ifndef INCLUDE_STEREOCENTERS_H
#define INCLUDE_STEREOCENTERS_H

#include <vector>
#include <set>
#include <algorithm>
#include <memory>
#include <boost/optional.hpp>

#include "common_typedefs.h"
#include "StdlibTypeAlgorithms.h"

// Detection algorithm headers
#include "AdjacencyList.h"
#include "Types/ElementTypeCollection.h" // Delib

/* TODO
 */

namespace MoleculeManip {

// Predeclaration
namespace Stereocenters {
class Stereocenter;
}

std::basic_ostream<char>& operator << (
  std::basic_ostream<char>& os,
  const std::shared_ptr<Stereocenters::Stereocenter>& stereocenterPtr
);

namespace Stereocenters {

class Stereocenter {
public:
/* Typedefs */
  enum class ChiralityConstraintTarget {
    Positive,
    Flat,
    Negative
  };

  using DihedralLimits = std::tuple<
    // Dihedral sequence i-j-k-l
    std::array<AtomIndexType, 4>,
    /* Lower and upper limit, where
     * 0 <= lower <= 180
     * lower <= upper <= 180
     */
    std::pair<double, double>
  >;

  using ChiralityConstraintPrototype = std::tuple<
    AtomIndexType, // i
    AtomIndexType, // j
    AtomIndexType, // k
    AtomIndexType, // l
    ChiralityConstraintTarget
  >;

/* Modification */
  //!  Assign this feature
  virtual void assign(const unsigned& assignment) = 0;

/* Information */
  /*!
   * Return the angle imposed by the underlying symmetry defined by three
   * involved atoms. It needs to be three-defined in order for the angle 
   * requested to be clearly defined in CNStereocenters and EZStereocenters.
   */
  virtual double angle(
    const AtomIndexType& i,
    const AtomIndexType& j,
    const AtomIndexType& k
  ) const = 0;

  /*!
   * Return whether this Stereocenter has been assigned or not
   * -> This leads to different behavior in DG! If unassigned, an Assignment is 
   *    chosen at random and adhered to during coordinate generation.
   */
  virtual boost::optional<unsigned> assigned() const = 0;

  //!  Return the number of possible assignments 
  virtual unsigned assignments() const = 0;

  //!  Return a list of chirality constraints
  // -> TODO Maybe need to integrate more information in the Symmetries if
  // precise 1-3 distances are not enough to fully specify the geometry
  virtual std::vector<ChiralityConstraintPrototype> chiralityConstraints() const = 0;

  /*!
   * Return the dihedral angle limits imposed by the underlying symmetries and
   * the current assignment.
   */
  virtual std::vector<DihedralLimits> dihedralLimits() const = 0;

  /*!
   * Return the set of center atoms (atoms that angle information is available 
   * on if asked as the central atom of an angle).
   */
  virtual std::set<AtomIndexType> involvedAtoms() const = 0;

  //!  Return a string specifying the type of stereocenter
  virtual std::string type() const = 0;

/* Operators */
  //!  Ostream operator for debugging
  friend std::basic_ostream<char>& MoleculeManip::operator << (
    std::basic_ostream<char>& os,
    const std::shared_ptr<MoleculeManip::Stereocenters::Stereocenter>& stereocenterPtr
  );
};

} // eo namespace Stereocenters

} // eo namespace

#endif
