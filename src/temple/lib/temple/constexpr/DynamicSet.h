/*!@file
 * @copyright ETH Zurich, Laboratory for Physical Chemistry, Reiher Group.
 *   See LICENSE.txt
 * @brief BTree-based std::set-like container (but max size is space allocated)
 *
 * A constexpr fixed-maximum-size managed set so that the type signature does
 * not change upon element insertion and deletion. STL parallel is std::set,
 * with the difference that here the maximum number of elements must be known at
 * compile time.
 */

#ifndef INCLUDE_MOLASSEMBLER_TEMPLE_CONSTEXPR_DYNAMIC_SET_H
#define INCLUDE_MOLASSEMBLER_TEMPLE_CONSTEXPR_DYNAMIC_SET_H

#include "temple/constexpr/BTree.h"

namespace temple {

/**
 * @brief Tree-based set
 *
 * @tparam T Value type of the set
 * @tparam nItems Maximum number of items to store in the set
 * @tparam LessThanPredicate
 */
template<
  typename T,
  size_t nItems,
  class LessThanPredicate = std::less<T>,
  class EqualityPredicate = std::equal_to<T>
> class DynamicSet {
private:
  using TreeType = BTree<T, 3, nItems, LessThanPredicate, EqualityPredicate>;

  TreeType _tree;

public:
  //! Dynamic set
  constexpr DynamicSet() = default;

  /*! @brief Constructor from existing ordered data
   *
   * Warning: These constructors expect ordered arrays!
   */
  template<
    template<typename, size_t> class ArrayType,
    size_t size
  > constexpr DynamicSet(const ArrayType<T, size>& items) {
    for(const auto& item : items) {
      _tree.insert(item);
    }
  }

  /*! @brief Check if the set contains an element.
   *
   * @complexity{@math{\Theta(N \log N)}}
   */
  PURITY_WEAK constexpr bool contains(const T& item) const {
    return _tree.contains(item);
  }

  /*! @brief Insertion an element into the set.
   *
   * @complexity{@math{\Theta(N \log N)}}
   */
  constexpr void insert(const T& item) {
    _tree.insert(item);
  }

  PURITY_WEAK constexpr Optional<const T&> getOption(const T& item) const {
    return _tree.getOption(item);
  }

  //! @brief Remove all elements of the set
  constexpr void clear() {
    _tree.clear();
  }

  using const_iterator = typename TreeType::const_iterator;

  PURITY_WEAK constexpr const_iterator begin() const {
    return _tree.begin();
  }

  PURITY_WEAK constexpr const_iterator end() const {
    return _tree.end();
  }

  PURITY_WEAK constexpr size_t size() const {
    return _tree.size();
  }

  PURITY_WEAK constexpr bool operator == (const DynamicSet& other) const {
    return _tree == other._tree;
  }

  PURITY_WEAK constexpr bool operator != (const DynamicSet& other) const {
    return !(
      _tree == other._tree
    );
  }

  PURITY_WEAK constexpr bool operator < (const DynamicSet& other) const {
    return _tree < other._tree;
  }

  PURITY_WEAK constexpr bool operator > (const DynamicSet& other) const {
    return other._tree < _tree;
  }
};

//! Helper function to create a DynamicSet specifying only the maximum size
template<
  size_t nItems,
  typename T,
  template<typename, size_t> class ArrayType,
  class LessThanPredicate = std::less<T>,
  class EqualityPredicate = std::equal_to<T>
> constexpr DynamicSet<T, nItems, LessThanPredicate> makeDynamicSet(
  const ArrayType<T, nItems>& array
) {
  return DynamicSet<T, nItems, LessThanPredicate, EqualityPredicate>(array);
}

} // namespace temple

#endif
