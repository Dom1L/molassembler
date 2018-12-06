/*!@file
 * @copyright ETH Zurich, Laboratory for Physical Chemistry, Reiher Group.
 *   See LICENSE.txt
 */

#define BOOST_FILESYSTEM_NO_DEPRECATED

#include "boost/filesystem.hpp"
#include "boost/range/iterator_range_core.hpp"
#include "boost/test/unit_test.hpp"

#include "temple/Stringify.h"
#include "temple/Functional.h"
#include "temple/constexpr/Numeric.h"

#include "molassembler/IO.h"
#include "molassembler/Cycles.h"
#include "molassembler/Molecule.h"
#include <iostream>
#include <array>

/* TODO add further tests for free functions
 */

struct ExpectationData {
  std::vector<unsigned> cycleSizes;

  ExpectationData(std::vector<unsigned>&& passCycleSizes)
    : cycleSizes(temple::sort(passCycleSizes)) {}
};

std::map<std::string, ExpectationData> decompositionData {
  {
    "fenestrane-3",
    {
      {3, 3, 3, 3}
    }
  },
  {
    "cyclooctatriene-fused-w-cyclopropane",
    {
      {3, 8}
    }
  },
  {
    "rotanes-5",
    {
      {3, 3, 3, 3, 5, 3}
    }
  },
  {
    "strained-db-aromatic-multicycles-1",
    {
      {5, 6, 6, 5, 6, 6}
    }
  },
  {
    "quadricyclane",
    {
      {3, 3, 4, 5, 5}
    }
  },
  {
    "aza-spiro-cyclo-3-3-with-cyclohexanyls",
    {
      {6, 6, 3, 3}
    }
  }
};

void readAndDecompose(const boost::filesystem::path& filePath) {
  using namespace molassembler;

  // Read the file
  auto mol = IO::read(filePath.string());

  Cycles cycles {mol.graph()};

  std::vector<unsigned> cycleSizes;
  for(const auto cyclePtr : cycles) {
    cycleSizes.emplace_back(Cycles::size(cyclePtr));
  }

  BOOST_CHECK(!cycleSizes.empty());

  auto findIter = decompositionData.find(filePath.stem().string());

  if(findIter != decompositionData.end()) {
    temple::inplace::sort(cycleSizes);

    BOOST_CHECK_MESSAGE(
      cycleSizes == findIter->second.cycleSizes,
      "Expected cycle sizes " << temple::condense(findIter->second.cycleSizes)
        << ", but got " << temple::condense(cycleSizes) << " for "
        << filePath.stem().string()
    );

    unsigned cycleSizeThreshold = temple::Math::ceil(temple::average(cycleSizes));

    for(
      const auto cyclePtr :
      boost::make_iterator_range(
        cycles.iteratorPair(
          Cycles::predicates::SizeLessThan(cycleSizeThreshold)
        )
      )
    ) {
      BOOST_CHECK_MESSAGE(
        Cycles::size(cyclePtr) < cycleSizeThreshold,
        "Expected edge set to have size less than " << cycleSizeThreshold
          << ", but got one with size " << Cycles::size(cyclePtr) << ": "
          << temple::stringify(Cycles::edgeVertices(cyclePtr))
      );
    }
  }

  std::cout << "'" << filePath.stem().string() << "' -> "
    << temple::condense(cycleSizes)
    << std::endl;
}

BOOST_AUTO_TEST_CASE(ringDecomposition) {
  for(
    const boost::filesystem::path& currentFilePath :
    boost::filesystem::recursive_directory_iterator("strained_organic_molecules")
  ) {
    BOOST_CHECK_NO_THROW(
      readAndDecompose(currentFilePath)
    );
  }
}
