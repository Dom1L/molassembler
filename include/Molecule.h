#ifndef INCLUDE_MOLECULE_H
#define INCLUDE_MOLECULE_H

// STL
#include <iostream>
#include <set>

// Delib
#include "Types/PositionCollection.h"
#include "Types/ElementTypeCollection.h"

// Custom headers
#include "AdjacencyList.h"
#include "EdgeList.h"
#include "StereocenterList.h"
#include "DistanceGeometry/DistanceBoundsMatrix.h"

namespace MoleculeManip {

class Molecule {
private:
/* private members */

  /* A Molecule conceptually contains a graph:
   * - Atoms are vertices (and thus have values)
   * - Bonds are edges (and thus weighted)
   * - The ensuing graph is
   *   - connected: a path from any node to any other exists
   *   - sparse: few edges present compared to the number of possible 
   *     edges -> use adjacency list instead of adjacency matrix
   */

  // The set of QC data on the atoms
  Delib::ElementTypeCollection _elements;
  Delib::PositionCollection _positions;
  // The information on interconnectedness of the atoms
  AdjacencyList _adjacencies;
  EdgeList _edges;
  StereocenterList _stereocenters;
  
  /* Private member functions */
  void _detectStereocenters();
  bool _validAtomIndex(const AtomIndexType& a) const;
  bool _validAtomIndices(
    const AtomIndexType& a,
    const AtomIndexType& b
  ) const;
  void _dumpGraphviz(std::ostream& os) const;
  std::vector<DistanceConstraint> _createConstraint(
    const std::vector<AtomIndexType>& chain
  ) const;

public:
/* Constructors */
  Molecule(
    const Delib::ElementType& a,
    const Delib::ElementType& b,
    const BondType& bondType
  ); 

  Molecule(
    const Delib::ElementTypeCollection& elements,
    const AdjacencyList& adjacencies,
    const EdgeList& edges
  );

  Molecule(
    const Delib::ElementTypeCollection& elements,
    const Delib::PositionCollection& positions,
    const AdjacencyList& adjacencies,
    const EdgeList& edges
  );

  AtomIndexType addAtom(
    const Delib::ElementType& elementType,
    const AtomIndexType& bondedToIndex,
    const BondType& bondType
  );

  void addBond(
    const AtomIndexType& a,
    const AtomIndexType& b,
    const BondType& bondType
  ); 

  void removeAtom(const AtomIndexType& a); 

  void removeBond(
    const AtomIndexType& a,
    const AtomIndexType& b
  );

  /* Information retrieval */
  Delib::ElementType getElementType(
    const AtomIndexType& a
  ) const;

  DistanceGeometry::DistanceBoundsMatrix getDistanceBoundsMatrix() const;

  int formalCharge(const AtomIndexType& a) const;
  int oxidationState(const AtomIndexType& a) const;
      
  AtomIndexType getNumAtoms() const;
  EdgeIndexType getNumBonds() const;
  const EdgeList& getEdgeList() const; 
  BondType getBondType(
    const AtomIndexType& a,
    const AtomIndexType& b
  ) const;
  unsigned hydrogenCount(const AtomIndexType& a) const;
  /*bool bond_exists(
    const AtomIndexType& a,
    const AtomIndexType& b
  ) const;
  bool bond_exists(
    const AtomIndexType& a,
    const AtomIndexType& b,
    const BondType& bond_type
  ) const;
  std::vector<
    std::pair<
      AtomIndexType,
      BondType
    >
  > get_bond_pairs(const AtomIndexType& a) const;
  */

  const AdjacencyList& getAdjacencyList() const {
    return _adjacencies;
  }

  std::vector<AtomIndexType> getBondedAtomIndices(
    const AtomIndexType& a
  ) const;

  std::pair<
    std::vector<AtomIndexType>, // the sorted list of substituent priorities
    std::set< // a set of pairs of AtomIndexTypes that are EQUAL
      std::pair<
        AtomIndexType,
        AtomIndexType
      >
    >
  > rankPriority(
    const AtomIndexType& a,
    const std::vector<AtomIndexType>& excludeAdjacent
  ) const;

  /* Testing */
  std::pair<bool, std::string> validate() const noexcept;

/* Operators */
  /* An efficient implementation of the following two is imperative.
   * Some ideas for fast differentiation can probably be found from the 
   * wikipedia category "Graph invariants"
   */
  bool operator == (const Molecule& b) const;
  bool operator != (const Molecule& b) const;

/* Friends */
  /* Output stream operator for easier debugging. */
  friend std::ostream& operator<<(std::ostream& os, const Molecule& mol);
};

}

#endif
