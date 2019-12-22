/*!@file
 * @copyright ETH Zurich, Laboratory for Physical Chemistry, Reiher Group.
 *   See LICENSE.txt
 * @brief Owning class storing all stereopermutators in a molecule
 *
 * Contains the declaration for a class that stores a list of all stereopermutators
 * in a molecule.
 */

#ifndef INCLUDE_MOLASSEMBLER_STEREOPERMUTATOR_LIST_H
#define INCLUDE_MOLASSEMBLER_STEREOPERMUTATOR_LIST_H

#include "boost/range/adaptor/map.hpp"
#include "boost/functional/hash.hpp"

#include "molassembler/AtomStereopermutator.h"
#include "molassembler/BondStereopermutator.h"

#include <unordered_map>

namespace Scine {
namespace molassembler {

/**
 * @brief Manages all stereopermutators that are part of a Molecule
 */
class MASM_EXPORT StereopermutatorList {
public:
//!@name Public types
//!@{
  using AtomMapType = std::unordered_map<AtomIndex, AtomStereopermutator>;
  using BondMapType = std::unordered_map<BondIndex, BondStereopermutator, boost::hash<BondIndex>>;
//!@}

//!@name Modification
//!@{
  /*! @brief Add a new AtomStereopermutator to the list
   *
   * @complexity{@math{\Theta(1)} amortized}
   *
   * @returns An iterator pointing to the added stereopermutator
   */
  AtomMapType::iterator add(AtomStereopermutator stereopermutator);

  /*! @brief Add a new BondStereopermutator to the list
   *
   * @complexity{@math{\Theta(1)} amortized}
   *
   * @returns An iterator pointing to the added stereopermutator
   */
  BondMapType::iterator add(BondStereopermutator stereopermutator);

  /*! @brief Apply an index mapping to the list of stereopermutators
   *
   * Applies the permutation to its maps, transforming the keys (atom and bond
   * indices) and all stereopermutators.
   *
   * @complexity{@math{\Theta(A + B)}}
   */
  void applyPermutation(const std::vector<AtomIndex>& permutation);

  /*! @brief Remove all stereopermutators
   *
   * @complexity{@math{\Theta(1)}}
   */
  void clear();

  /*! @brief Remove all stereopermutators on bonds
   *
   * @complexity{@math{\Theta(1)}}
   */
  void clearBonds();

  /*! @brief Fetch a reference-option to an AtomStereopermutator
   *
   * @complexity{@math{\Theta(1)}}
   */
  boost::optional<AtomStereopermutator&> option(AtomIndex index);

  /*! @brief Fetch a reference-option to a BondStereopermutator
   *
   * @complexity{@math{\Theta(1)}}
   */
  boost::optional<BondStereopermutator&> option(const BondIndex& edge);

  /*! @brief Communicates the removal of a vertex index to all stereopermutators in the list
   *
   * Removing a vertex invalidates some vertex descriptors, which are used
   * liberally in the stereopermutator classes. This function ensures that
   * vertex descriptors are valid throughout.
   *
   * @complexity{@math{\Theta(A + B)}}
   */
  void propagateVertexRemoval(AtomIndex removedIndex);

  /*! @brief Removes the AtomStereopermutator on a specified index
   *
   * @complexity{@math{\Theta(1)}}
   */
  void remove(AtomIndex index);

  /*! @brief Removes the BondStereopermutator on a specified edge
   *
   * @complexity{@math{\Theta(1)}}
   */
  void remove(const BondIndex& edge);

  /*! @brief Removes the AtomStereopermutator on a specified index, if present
   *
   * @complexity{@math{\Theta(1)}}
   */
  void try_remove(AtomIndex index);

  /*! @brief Removes the BondStereopermutator on a specified edge, if present
   *
   * @complexity{@math{\Theta(1)}}
   */
  void try_remove(const BondIndex& edge);
//!@}

//!@name Information
//!@{
  /*! @brief Modular comparison with another StereopermutatorList using a bitmask
   *
   * @complexity{@math{O(A + B)}}
   */
  bool compare(
    const StereopermutatorList& other,
    AtomEnvironmentComponents componentBitmask
  ) const;

  /*! @brief Returns true if there are no stereopermutators
   *
   * @complexity{@math{\Theta(1)}}
   */
  bool empty() const;

  /*! @brief Returns true if there are any stereopermutators with zero possible assignments
   *
   * @complexity{@math{O(A + B)}}
   */
  bool hasZeroAssignmentStereopermutators() const;

  /*! @brief Returns true if there are unassigned stereopermutators
   *
   * @complexity{@math{O(A + B)}}
   */
  bool hasUnassignedStereopermutators() const;

  /*! @brief Fetch a const ref-option to an AtomStereopermutator, if present
   *
   * @complexity{@math{\Theta(1)}}
   */
  boost::optional<const AtomStereopermutator&> option(AtomIndex index) const;

  /*! @brief Fetch a const ref-option to a BondStereopermutator, if present
   *
   * @complexity{@math{\Theta(1)}}
   */
  boost::optional<const BondStereopermutator&> option(const BondIndex& edge) const;

  /*! @brief Returns the number of AtomStereopermutators
   *
   * @complexity{@math{\Theta(1)}}
   */
  unsigned A() const;

  /*! @brief Returns the number of BondStereopermutators
   *
   * @complexity{@math{\Theta(1)}}
   */
  unsigned B() const;

  /*! @brief Combined size of atom and bond-stereopermutator lists
   *
   * @complexity{@math{\Theta(1)}}
   */
  unsigned size() const;
//!@}

//!@name Ranges (not thread-safe)
//!@{
  /*! @brief Returns an iterable object with modifiable atom stereopermutator references
   *
   * @complexity{@math{\Theta(1)}}
   */
  boost::range_detail::select_second_mutable_range<AtomMapType> atomStereopermutators();

  /*! @brief Returns an iterable object with unmodifiable atom stereopermutator references
   *
   * @complexity{@math{\Theta(1)}}
   */
  boost::range_detail::select_second_const_range<AtomMapType> atomStereopermutators() const;

  /*! @brief Returns an iterable object with modifiable bond stereopermutator references
   *
   * @complexity{@math{\Theta(1)}}
   */
  boost::range_detail::select_second_mutable_range<BondMapType> bondStereopermutators();

  /*! @brief Returns an iterable object with unmodifiable bond stereopermutator references
   *
   * @complexity{@math{\Theta(1)}}
   */
  boost::range_detail::select_second_const_range<BondMapType> bondStereopermutators() const;
//!@}

//!@name Operators
//!@{
  /*! @brief Strict equality comparison
   *
   * @complexity{@math{O(A + B)}}
   */
  bool operator == (const StereopermutatorList& other) const;
  //! Inverts @p operator ==
  bool operator != (const StereopermutatorList& other) const;
//!@}

private:
  //! The underlying storage for atom stereopermutators
  AtomMapType _atomStereopermutators;

  //! The underlying storage for bond stereopermutators
  BondMapType _bondStereopermutators;
};

} // namespace molassembler
} // namespace Scine
#endif
