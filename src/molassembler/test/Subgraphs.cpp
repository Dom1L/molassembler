/*!@file
 * @copyright ETH Zurich, Laboratory for Physical Chemistry, Reiher Group.
 *   See LICENSE.txt
 */

#include <boost/test/unit_test.hpp>

#include "molassembler/Subgraphs.h"
#include "molassembler/Molecule.h"
#include "molassembler/Patterns.h"
#include "molassembler/IO.h"

#include <iostream>
#include "temple/Stringify.h"

using namespace Scine;
using namespace molassembler;

BOOST_AUTO_TEST_CASE(SubgraphBasic) {
  const Molecule neopentane = IO::read("isomorphisms/neopentane.mol");
  const Molecule methyl = patterns::methyl().first;

  const auto mappings = subgraphs::maximum(methyl, neopentane);
  BOOST_CHECK_MESSAGE(
    mappings.size() == 4,
    "Expected four mappings total, got " << mappings.size() << " instead"
  );
}