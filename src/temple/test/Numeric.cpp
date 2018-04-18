#define BOOST_TEST_MODULE NumericTestsModule
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include "temple/Containers.h"
#include "temple/constexpr/Numeric.h"
#include "temple/Random.h"

BOOST_AUTO_TEST_CASE(kahanSummation) {
  for(unsigned nTest = 0; nTest < 100; nTest++) {
    const unsigned N = 100;
    const unsigned magnitudeSpread = 20;
    const auto randomNumbers = temple::random.getN<double>(
      std::pow(10, - static_cast<double>(magnitudeSpread) / 2),
      std::pow(10, static_cast<double>(magnitudeSpread) / 2),
      N
    );

    const double reduceSum = temple::reduce(
      randomNumbers,
      0.0,
      std::plus<double>()
    );

    const double kahanSum = temple::kahanSum(randomNumbers);

    /* Reference sum with long doubles, I know an alternative implementation of
     * standard reduce summation with long intermediates would have done the
     * trick too but I'm lazy and this is just a test
     */
    const auto addedPrecision = temple::cast<long double>(randomNumbers);
    const double longSum = temple::reduce(
      addedPrecision,
      0.0l,
      std::plus<long double>()
    );

    // Kahan summation should be equally or more accurate than the standard reduction
    BOOST_CHECK_MESSAGE(
      std::fabs(longSum - reduceSum) >= std::fabs(longSum - kahanSum),
      "Kahan summation is less accurate than standard reduce sum! "
      << "long: " << longSum << ", kahan: " << kahanSum 
      << ", reduce: " << reduceSum << ". Absolute deviation from long sum: "
      << "kahan:" << std::fabs(longSum - kahanSum) << ", "
      << "reduce: " << std::fabs(longSum - reduceSum)
    );
  }
}

BOOST_AUTO_TEST_CASE(numericAverageStdDev) {
  const std::vector<double> values {29, 30, 31, 32, 33};

  BOOST_CHECK(temple::average(values) == 31); 
  BOOST_CHECK(
    std::fabs(temple::stddev(values) - std::sqrt(2)) 
    < 1e-10
  );
}
