/*!@file
 * @copyright ETH Zurich, Laboratory for Physical Chemistry, Reiher Group.
 *   See LICENSE.txt
 * @brief Wrapper class to typify Angstrom scale positional information
 *
 * Provides a class that exists purely to strongly separate position collections
 * in bohr units and angstrom units in library interfaces
 */

#ifndef INCLUDE_MOLASSEMBLER_ANGSTROM_POSITIONS_H
#define INCLUDE_MOLASSEMBLER_ANGSTROM_POSITIONS_H

#include "Delib/PositionCollection.h"

#include "molassembler/Types.h"

namespace molassembler {

/**
 * @brief A wrapper class around Delib's PositionCollection to emphasize that
 *   the positions stored therein are in Angstrom
 */
class AngstromWrapper {
public:
  Delib::PositionCollection positions;

  AngstromWrapper() = default;
  explicit AngstromWrapper(unsigned N);
  explicit AngstromWrapper(
    Delib::PositionCollection pos,
    LengthUnit lengthUnit = LengthUnit::Bohr
  );

  /*!
   * @brief Fetch a bohr representation of the wrapped positions
   *
   * @warning After calling this function, you should not reuse the
   * corresponding instance, as the underlying positions have been converted to
   * bohr.
   */
  Delib::PositionCollection getBohr();

private:
  bool _invalidated = false;
};

} // namespace molassmbler

#endif
