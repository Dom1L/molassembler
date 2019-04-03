/*!@file
 * @copyright ETH Zurich, Laboratory for Physical Chemistry, Reiher Group.
 *   See LICENSE.txt
 * @brief Algorithms to determine local symmetry from graph information
 *
 * Declarations for the general interface with which a number of classes can
 * determine the local geometry that a specific arrangement of atoms should
 * have.
 */

#ifndef INCLUDE_MOLASSEMBLER_LOCAL_GEOMETRY_MODEL_H
#define INCLUDE_MOLASSEMBLER_LOCAL_GEOMETRY_MODEL_H

#include "boost/optional/optional_fwd.hpp"

#include "Utils/Geometry/ElementTypes.h"
#include "chemical_symmetries/Names.h"

#include "molassembler/RankingInformation.h"

#include <map>

namespace Scine {

namespace molassembler {

// Forward-declarations
class OuterGraph;

/**
 * @brief Methods to determine local geometries of atoms based on limited graph
 *   information
 */
namespace LocalGeometry {

/* Typedefs */
struct BindingSiteInformation {
  unsigned L, X;

  std::vector<Scine::Utils::ElementType> elements;
  /* Only one bond type is needed - If the ligand consists of a single atom,
   * then we only need to store one bond. If the ligand consists of multiple
   * atoms, then the BondType is Eta.
   */
  BondType bondType;

  BindingSiteInformation() = default;
  BindingSiteInformation(
    const unsigned passL,
    const unsigned passX,
    std::vector<Scine::Utils::ElementType> passElements,
    const BondType passBondType
  ) : L(passL), X(passX), elements {std::move(passElements)}, bondType {passBondType} {}
};

/*!
 * @brief Mapping of bond type to a floating-point weight
 * @todo Eta bonds have weight 0, is this a good idea?
 */
extern const std::map<BondType, double> bondWeights;

/**
 * @brief Calculates the formal charge on a main group-element atom.
 *
 * @param graph The graph to which the atom index belongs
 * @param index The atom index for which to calculate the formal charge
 *
 * @warning This is an awful function and should be avoided in any sort of
 *   important calculation.
 *
 * @warning This will yield nonsense if the bond orders in your graph are unset.
 *
 * @warning This function may work sometimes for organic surroundings. That's
 *   how confident we are in this function.
 *
 * @return 0 for non-main group elements, the formal charge otherwise
 */
int formalCharge(
  const OuterGraph& graph,
  AtomIndex index
);

/* Models */
boost::optional<Symmetry::Name> vsepr(
  Scine::Utils::ElementType centerAtomType,
  const std::vector<BindingSiteInformation>& sites,
  int formalCharge
);

Symmetry::Name firstOfSize(unsigned size);


/* Tiered geometry determination function */
std::vector<LocalGeometry::BindingSiteInformation> reduceToSiteInformation(
  const OuterGraph& molGraph,
  AtomIndex index,
  const RankingInformation& ranking
);

boost::optional<Symmetry::Name> inferSymmetry(
  const OuterGraph& graph,
  AtomIndex index,
  const RankingInformation& ranking
);

} // namespace LocalGeometry

} // namespace molassembler

} // namespace Scine

#endif
