#ifndef INCLUDE_DG_MOLECULE_SPATIAL_MODEL_H
#define INCLUDE_DG_MOLECULE_SPATIAL_MODEL_H

#include "Molecule.h"
#include "DistanceGeometry/ValueBounds.h"

/*! @file
 *
 * The molecular graph with all its conformational specifications via
 * stereocenter assignments must be transformed into a spatial model that
 * describes its internal degrees of freedom in a manner translatable to
 * pairwise distance bounds for the distance geometry algorithm. This file
 * contains the class declaration for that spatial model.
 */

namespace MoleculeManip {

namespace DistanceGeometry {

template<int size>
std::array<AtomIndexType, size> orderedIndexSequence(
  const std::array<AtomIndexType, size>& source
) {
  if(source.front() > source.back()) {
    auto copy = source;
    std::reverse(
      copy.begin(),
      copy.end()
    );
    return copy;
  }

  return source;
}

template<int size>
std::array<AtomIndexType, size> orderedIndexSequence(
  std::initializer_list<AtomIndexType>& initializer
) {
  assert(size == initializer.size());
  return orderedIndexSequence(
    std::array<AtomIndexType, size>(initializer)
  );
}

/*!
 * Keeps a record of the internal dimension bounds that a molecular graph is
 * interpreted as and permits the generation of a distance bounds matrix.
 */
class MoleculeSpatialModel {
public:
/* Typedefs */
  struct ModelGraphWriter;

  enum class DistanceMethod {
    Uniform,
    UFFLike
  };

private:
  // Closures
  const Molecule& _molecule;

  // Mutable state
  std::map<
    std::array<AtomIndexType, 2>,
    ValueBounds
  > _bondBounds;
  std::map<
    std::array<AtomIndexType, 3>,
    ValueBounds
  > _angleBounds;
  std::map<
    std::array<AtomIndexType, 4>,
    ValueBounds
  > _dihedralBounds;

  std::map<
    AtomIndexType,
    std::shared_ptr<Stereocenters::Stereocenter>
  > _stereocenterMap;

public:
/* Static constants */
  /*! Relative bond distance variance, 0.0x meaning x% variance. Must fulfill
   * 0 < x << 1
   */
  static constexpr double bondRelativeVariance = 0.01; 
  //! Absolute angle variance in radians. Must fulfill 0 < x << M_PI
  static constexpr double angleAbsoluteVariance = M_PI / 36; // ~ 5°

  static double spiroCrossAngle(const double& alpha, const double& beta);

/* Constructor */
  MoleculeSpatialModel(
    const Molecule& molecule,
    const DistanceMethod& distanceMethod = DistanceMethod::UFFLike
  );

/* Modification */
  /*! 
   * Sets the bond bounds to the model. Does not check if previous information
   * exists
   */
  void setBondBounds(
    const std::array<AtomIndexType, 2>& bondIndices,
    const double& centralValue,
    const double& relativeVariance
  );

  /*!
   * Adds the angle bounds to the model, but only if the information for that
   * set of indices does not exist yet.
   */
  void setAngleBoundsIfEmpty(
    const std::array<AtomIndexType, 3>& angleIndices,
    const double& centralValue,
    const double& absoluteVariance
  );

  /*!
   * Adds the dihedral bounds to the model, but only if the information for that
   * set of indices does not exist yet.
   */
  void setDihedralBoundsIfEmpty(
    const std::array<AtomIndexType, 4>& dihedralIndices,
    const double& lower,
    const double& upper
  );

  /*! 
   * Adds [0, 2π] default dihedrals to the model. Use immediately before
   * calling makeDistanceBounds if you want default dihedrals modeled in the
   * distance bounds as well. In principle, the default dihedral distances are
   * inferable from the existing information using bound smoothing, but this 
   * fashion is probably significantly faster.
   */
  void addDefaultDihedrals();

  using BoundList = std::vector<
    std::tuple<AtomIndexType, AtomIndexType, ValueBounds>
  >;

  BoundList makeBoundList() const;

  std::vector<
    Stereocenters::ChiralityConstraintPrototype
  > getChiralityPrototypes() const;

  void dumpDebugInfo() const;
  void writeGraphviz(const std::string& filename) const;
};

} // namespace DistanceGeometry

} // namespace MoleculeManip

#endif
