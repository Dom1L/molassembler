// Copyright ETH Zurich, Laboratory for Physical Chemistry, Reiher Group.
// See LICENSE.txt for details.

#ifndef INCLUDE_MOLASSEMBLER_CONFORMER_GENERATION_H
#define INCLUDE_MOLASSEMBLER_CONFORMER_GENERATION_H

#include "molassembler/Types.h"
#include "boost_outcome/outcome.hpp"
#include <vector>

// Forward-declarations
namespace Delib {
class PositionCollection;
} // namespace Delib

/*!@file
 *
 * @brief Interface for the generation of new conformations of Molecules
 */

namespace molassembler {

namespace outcome = BOOST_OUTCOME_V2_NAMESPACE;

// Forward-declarations
class Molecule;

namespace DistanceGeometry {

/**
 * @brief A configuration object for distance geometry runs with sane defaults
 */
struct Configuration {
  /**
   * @brief Choose for how many atoms to re-smooth the distance bounds after
   *   a distance choice
   */
  Partiality partiality {Partiality::FourAtom};

  /**
   * @brief Limit the maximum number of refinement steps
   *
   * The default value is typically enough for medium-sized systems, but may
   * need to be incremented for large systems.
   */
  unsigned refinementStepLimit {10000};

  /**
   * @brief Sets the gradient at which a refinement is considered complete
   *
   * The default value is fairly tight, and can be loosened if faster results
   * are desired and looser local symmetries are tolerable.
   */
  double refinementGradientTarget {1e-5};

  /**
   * @brief Sets the maximum allowed ratio of failures / (# desired conformers)
   *
   * The default value is loose, and allows many failures. It can be tightened
   * (towards lower values) to progressively loosen attempted spatial modelling
   * faster and admit defeat quicker.
   */
  double failureRatio {2};
};

} // namespace DistanceGeometry

/*!
 * @brief Generate multiple sets of positional data for a Molecule
 *
 * In the case of a molecule that does not have unassigned stereopermutators,
 * this is akin to generating a conformational ensemble. If there are
 * unassigned stereopermutators, these are assigned at random (consistent with
 * relative statistical occurrences of stereopermutations) for each structure.
 *
 * @param molecule The molecule for which to generate sets of three-dimensional
 *   positions. This molecule may not contain stereopermutators with zero
 *   assignments.
 * @param numStructures The number of desired structures to generate
 * @param configuration The configuration object to control Distance Geometry
 *   in detail. The defaults are usually fine.
 *
 * @returns A result type which may or may not contain a vector of
 *   PositionCollections (in Bohr length units). The result type is much like
 *   an optional, except that in the error case it carries data about the error
 *   in order to help diagnose possible mistakes made in the molecular graph
 *   specification.
 */
outcome::result<
  std::vector<Delib::PositionCollection>
> generateEnsemble(
  const Molecule& molecule,
  unsigned numStructures,
  const DistanceGeometry::Configuration& configuration = DistanceGeometry::Configuration {}
);

/*! Generate a 3D structure of a Molecule
 *
 * @param molecule The molecule for which to generate three-dimensional
 *   positions. This molecule may not contain stereopermutators with zero
 *   assignments.
 * @param configuration The configuration object to control Distance Geometry
 *   in detail. The defaults are usually fine.
 *
 * @returns A result type which may or may not contain a PositionCollection (in
 * Bohr length units). The result type is much like an optional, except that in
 * the error case it carries data about the error in order to help diagnose
 * possible mistakes made in the molecular graph specification.
 */
outcome::result<Delib::PositionCollection> generateConformation(
  const Molecule& molecule,
  const DistanceGeometry::Configuration& configuration = DistanceGeometry::Configuration {}
);

} // namespace molassembler

#endif
