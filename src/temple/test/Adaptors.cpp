// Copyright ETH Zurich, Laboratory for Physical Chemistry, Reiher Group.
// See LICENSE.txt for details.

#include <boost/test/unit_test.hpp>

#include "temple/Adaptors/All.h"
#include "temple/Functional.h"
#include "temple/Stringify.h"
#include "temple/constexpr/Numeric.h"

#include <iostream>

template<class Container>
std::size_t iteratorDistance(const Container& container) {
  return std::distance(std::begin(container), std::end(container));
}

BOOST_AUTO_TEST_CASE(pairAdaptorTests) {
  const std::vector<unsigned> i {5, 3, 9, 11}, j {3, 4};

  auto adjacents = temple::adaptors::sequentialPairs(i);

  BOOST_CHECK(adjacents.size() == 3);
  BOOST_CHECK(iteratorDistance(adjacents) == adjacents.size());

  BOOST_CHECK(
    temple::sum(
      temple::adaptors::transform(adjacents, std::plus<>())
    ) == 8 + 12 + 20
  );

  auto singlePairs = temple::adaptors::allPairs(i);

  BOOST_CHECK(singlePairs.size() == 6);
  BOOST_CHECK(
    iteratorDistance(singlePairs) == singlePairs.size()
  );

  BOOST_CHECK(
    temple::sum(
      temple::adaptors::transform(singlePairs, std::plus<>())
    ) == 8 + 14 + 16 + 12 + 14 + 20
  );

  auto twoPairs = temple::adaptors::allPairs(i, j);

  BOOST_CHECK(twoPairs.size() == 8);
  BOOST_CHECK(
    iteratorDistance(twoPairs) == twoPairs.size()
  );

  BOOST_CHECK(
    temple::sum(
      temple::adaptors::transform(twoPairs, std::plus<>())
    ) == 8 + 9 + 6 + 7 + 12 + 13 + 14 + 15
  );
}

BOOST_AUTO_TEST_CASE(iotaAdaptorTests) {
  auto a = temple::adaptors::range(5u);

  BOOST_CHECK(a.size() == 5);
  BOOST_CHECK(iteratorDistance(a) == a.size());
  BOOST_CHECK(temple::sum(a) == 10u);

  auto b = temple::adaptors::range(4u, 7u);

  BOOST_CHECK(b.size() == 3);
  BOOST_CHECK(iteratorDistance(b) == b.size());
  BOOST_CHECK(temple::sum(b) == 15u);
}

BOOST_AUTO_TEST_CASE(zipAdaptorTests) {
  const std::vector<unsigned> i {5, 3, 9, 11}, j {3, 4};

  auto zipRange = temple::adaptors::zip(i, j);

  BOOST_CHECK(zipRange.size() == 2);
  BOOST_CHECK(iteratorDistance(zipRange) == zipRange.size());
  BOOST_CHECK(
    temple::sum(
      temple::adaptors::transform(zipRange, std::plus<>())
    ) == 15u
  );
}

BOOST_AUTO_TEST_CASE(transformAdaptorTests) {
  const std::vector<unsigned> i {5, 3, 9, 11};

  auto transformRange = temple::adaptors::transform(
    i,
    [](unsigned x) -> int {return static_cast<int>(x) - 10;}
  );

  BOOST_CHECK(transformRange.size() == 4);
  BOOST_CHECK(iteratorDistance(transformRange) == transformRange.size());
  BOOST_CHECK(
    temple::sum(transformRange) == static_cast<int>(temple::sum(i)) - 4 * 10
  );
}

BOOST_AUTO_TEST_CASE( enumerateTests) {
  std::vector<unsigned> testVec {5, 2, 3, 4};

  bool pass = true;
  for(const auto& enumPair : temple::adaptors::enumerate(testVec)) {
    if(testVec.at(enumPair.index) != enumPair.value) {
      pass = false;
      break;
    }
  }

  BOOST_CHECK(pass);

  auto weirdSum = temple::sum(
    temple::map(
      temple::adaptors::enumerate(testVec),
      [](const auto& enumPair) -> unsigned {
        return enumPair.index + enumPair.value;
      }
    )
  );

  BOOST_CHECK(weirdSum == 5 + 3 + 5 + 7);
}


BOOST_AUTO_TEST_CASE(compoundAdaptorOwnership) {
  auto pairsOfRange = temple::adaptors::allPairs(
    temple::adaptors::range(4u)
  );

  auto selfOwningRange = temple::adaptors::range(4u);
  auto referenceOwningPairs = temple::adaptors::allPairs(selfOwningRange);

  auto checkPairs = [](const auto& rangeObject) -> void {
    BOOST_CHECK(rangeObject.size() == 6);
    BOOST_CHECK(iteratorDistance(rangeObject) == rangeObject.size());
    BOOST_CHECK(
      temple::invoke(std::plus<>(), *std::begin(rangeObject)) == 1u
    );

    BOOST_CHECK(
      temple::sum(
        temple::adaptors::transform(
          temple::adaptors::allPairs(
            temple::adaptors::range(4u)
          ),
          [](const unsigned i, const unsigned j) -> unsigned {
            return i * j;
          }
        )
      ) == 11u
    );
  };

  checkPairs(pairsOfRange);
  checkPairs(referenceOwningPairs);

  std::vector<unsigned> i {1, 4, 9}, j {5, 2};

  auto pairFromTwoReferences = temple::adaptors::allPairs(i, j);
  auto pairFromTwoRValues = temple::adaptors::allPairs(
    std::vector<unsigned> {1, 4, 9},
    std::vector<unsigned> {5, 2}
  );
  auto pairFromMixed = temple::adaptors::allPairs(
    std::vector<unsigned> {1, 4, 9},
    j
  );

  auto checkTwoPairs = [](const auto& rangeObject) -> void {
    BOOST_CHECK(rangeObject.size() == 6);
    BOOST_CHECK(iteratorDistance(rangeObject) == rangeObject.size());
    BOOST_CHECK(
      temple::invoke(std::plus<>(), *std::begin(rangeObject)) == 6
    );

    BOOST_CHECK(
      temple::sum(
        temple::adaptors::transform(
          rangeObject,
          std::plus<>()
        )
      ) == 6 + 3 + 9 + 6 + 14 + 11
    );
  };

  checkTwoPairs(pairFromTwoReferences);
  checkTwoPairs(pairFromTwoRValues);
  checkTwoPairs(pairFromMixed);

}

BOOST_AUTO_TEST_CASE(adaptorShortRanges) {
  auto checkRangeLength = [](
    const auto& rangeObject,
    const unsigned expectedSize,
    const std::string& description
  ) {
    BOOST_CHECK_MESSAGE(
      rangeObject.size() == expectedSize,
      description << " size is " << rangeObject.size() << ", expected "
        << expectedSize
    );
    BOOST_CHECK_MESSAGE(
      iteratorDistance(rangeObject) == rangeObject.size(),
      description << " iterator distance is " << iteratorDistance(rangeObject)
        << ", expected equal to size (" << rangeObject.size() << ")"
    );
  };

  checkRangeLength(
    temple::adaptors::allPairs(std::vector<unsigned> {4}),
    0,
    "single-element all-pairs"
  );

  checkRangeLength(
    temple::adaptors::allPairs(std::vector<unsigned> {}),
    0,
    "no-element all-pairs"
  );

  checkRangeLength(
    temple::adaptors::allPairs(
      std::vector<unsigned> {4},
      std::vector<unsigned> {6}
    ),
    1,
    "one-one all-pairs"
  );

  checkRangeLength(
    temple::adaptors::allPairs(
      std::vector<unsigned> {},
      std::vector<unsigned> {6}
    ),
    0,
    "none-one all-pairs"
  );

  checkRangeLength(
    temple::adaptors::allPairs(
      std::vector<unsigned> {},
      std::vector<unsigned> {}
    ),
    0,
    "none-none all-pairs"
  );

  checkRangeLength(
    temple::adaptors::sequentialPairs(std::vector<unsigned> {4}),
    0,
    "one-element sequential pairs"
  );

  checkRangeLength(
    temple::adaptors::sequentialPairs(std::vector<unsigned> {}),
    0,
    "no-element sequential pairs"
  );
}