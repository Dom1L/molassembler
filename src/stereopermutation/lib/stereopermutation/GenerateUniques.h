#ifndef INCLUDE_MOLASSEMBLER_STEREOPERMUTATIONS_GENERATE_UNIQUES_H
#define INCLUDE_MOLASSEMBLER_STEREOPERMUTATIONS_GENERATE_UNIQUES_H

#include "stereopermutation/Stereopermutation.h"

/*! @file
 *
 * @brief Generate non-superposable atom-centered stereopermutations
 *
 * Main entry point into the library. From here, a set of rotationally unique
 * assignments for a specific assignment can be generated, with or without
 * counting the relative weights of unique assignments.
 */

namespace stereopermutation {

//! Whether a stereopermutation has trans arranged linked substituents
bool hasTransArrangedPairs(
  const Stereopermutation& assignment,
  Symmetry::Name symmetryName
);

/*!
 * Generate the set of rotationally unique assignments for a given assignment.
 * By default removes trans-spanning groups (where a linked group's
 * directly bonded atoms span an angle of 180°).
 *
 * NOTE: Gives NO guarantees as to satisfiability (if assignments can be
 *  fulfilled with real ligands)
 * E.g. M (A-A)_3 generates a trans-trans-trans assignment, which is extremely
 *  hard to find actual ligands for that work.
 * The satisfiability of assignments must be checked before trying to embed
 *  structures with completely nonsensical constraints. Perhaps restrict A-A
 *  ligands with bridge length 4 (chelating atoms included), maybe even up to 6
 *  to cis arrangements. Xantphos (with bridge length 7) is the smallest
 *  trans-spanning ligand mentioned in Wikipedia.
 */
std::vector<Stereopermutation> uniques(
  const Stereopermutation& initial,
  Symmetry::Name symmetryName,
  bool removeTransSpanningGroups = false
);

//! Data class for uniqueStereopermutations including weights
struct StereopermutationsWithWeights {
  std::vector<Stereopermutation> assignments;
  std::vector<unsigned> weights;
};

/*!
 * Returns the set of rotationally unique assignments including absolute
 * occurrence counts.
 */
StereopermutationsWithWeights uniquesWithWeights(
  const Stereopermutation& initial,
  Symmetry::Name symmetryName,
  bool removeTransSpanningGroups = false
);

} // namespace stereopermutation

#endif
