#define BOOST_TEST_MODULE RankingTreeTestsModule
#define BOOST_FILESYSTEM_NO_DEPRECATED

#include "boost/filesystem.hpp"
#include "boost/graph/isomorphism.hpp"
#include "boost/test/unit_test.hpp"

#include "temple/constexpr/Bitmask.h"
#include "temple/Containers.h"
#include "temple/Stringify.h"

#include "molassembler/detail/StdlibTypeAlgorithms.h"
#include "molassembler/Cycles.h"
#include "molassembler/GraphAlgorithms.h"
#include "molassembler/IO.h"
#include "molassembler/RankingTree.h"
#include "molassembler/StereocenterList.h"

#include <random>
#include <fstream>

using namespace molassembler;

const std::string directoryPrefix = "test_files/ranking_tree_molecules/";

bool isStereocenter(
  const Molecule& molecule,
  const GraphType::edge_descriptor& e,
  const unsigned numPermutations,
  const boost::optional<unsigned>& assignment
) {
  auto stereocenterOption = molecule.getStereocenterList().option(e);

  if(!stereocenterOption) {
    std::cout << "No stereocenter on vertices " << temple::stringify(
      molecule.vertices(e)
    ) << "\n";
    return false;
  }

  if(stereocenterOption->numStereopermutations() != numPermutations) {
    std::cout << "Bond stereocenter on "
      << temple::stringify(
        molecule.vertices(e)
      )
      << " has " << stereocenterOption->numStereopermutations()
      << " stereopermutations, not " << numPermutations << "\n";
    return false;
  }

  if(assignment) {
    if(stereocenterOption->assigned() != assignment.value()) {
      std::cout << "Bond stereocenter on "
        << temple::stringify(
          molecule.vertices(e)
        )
        << " is assigned "
        << (
          stereocenterOption->assigned()
          ? std::to_string(stereocenterOption->assigned().value())
          : "u"
        )
        << ", not " << assignment.value() << "\n";
      return false;
    }
  }

  return true;
}

bool isStereocenter(
  const Molecule& molecule,
  AtomIndexType i,
  const unsigned numPermutations,
  const boost::optional<unsigned>& assignment
) {
  auto stereocenterOption = molecule.getStereocenterList().option(i);

  if(!stereocenterOption) {
    std::cout << "No stereocenter on atom index " << i << "\n";
    return false;
  }

  if(stereocenterOption->numStereopermutations() != numPermutations) {
    std::cout << "Atom stereocenter on " << i << " has "
      << stereocenterOption->numStereopermutations() << " stereopermutations, not "
      << numPermutations << "\n";
    return false;
  }

  if(assignment) {
    if(stereocenterOption->assigned() != assignment.value()) {
      std::cout << "Atom stereocenter on " << i << " is assigned "
        << (
          stereocenterOption->assigned()
          ? std::to_string(stereocenterOption->assigned().value())
          : "u"
        ) << ", not " << assignment.value() << "\n";
      return false;
    }
  }

  return true;
}

bool isStereogenic(
  const Molecule& molecule,
  AtomIndexType i
) {
  auto stereocenterOption = molecule.getStereocenterList().option(i);

  if(!stereocenterOption) {
    return false;
  }

  if(stereocenterOption->numStereopermutations() <= 1) {
    return false;
  }

  return true;
}

void writeExpandedTree(
  const std::string& fileName,
  const AtomIndexType expandOnIndex
) {
  auto molecule = IO::read(
    directoryPrefix + fileName
  );

  auto expandedTree = RankingTree(
    molecule.getGraph(),
    molecule.getCycleData(),
    molecule.getStereocenterList(),
    molecule.dumpGraphviz(),
    expandOnIndex,
    {},
    RankingTree::ExpansionOption::Full
  );

  std::ofstream dotFile(fileName + ".dot");
  dotFile << expandedTree.dumpGraphviz();
  dotFile.close();
}

BOOST_AUTO_TEST_CASE(TreeExpansionAndSequenceRuleOneTests) {
  using namespace std::string_literals;


  // Basic tests

  /* P-92.2.1.1.2 Spheres I and II */
  auto exampleOne = IO::read(
    directoryPrefix + "2R-2-chloropropan-1-ol.mol"s
  );

  auto exampleOneExpanded = RankingTree(
    exampleOne.getGraph(),
    exampleOne.getCycleData(),
    exampleOne.getStereocenterList(),
    exampleOne.dumpGraphviz(),
    2,
    {},
    RankingTree::ExpansionOption::Full
  );

  auto exampleTwo = IO::read(
    directoryPrefix + "2S-23-dichloropropan-1-ol.mol"
  );

  auto exampleTwoExpanded = RankingTree(
    exampleTwo.getGraph(),
    exampleTwo.getCycleData(),
    exampleTwo.getStereocenterList(),
    exampleTwo.dumpGraphviz(),
    3,
    {},
    RankingTree::ExpansionOption::Full
  );

  // P. 92.2.2 Sequence subrule 1b: Priority due to duplicate atoms
  // Cycle and multiple-bond splitting
  auto exampleThree = IO::read(
    directoryPrefix + "1S5R-bicyclo-3-1-0-hex-2-ene.mol"
  );

  auto exampleThreeExpanded = RankingTree(
    exampleThree.getGraph(),
    exampleThree.getCycleData(),
    exampleThree.getStereocenterList(),
    exampleThree.dumpGraphviz(),
    0,
    {},
    RankingTree::ExpansionOption::Full
  );

  auto exampleThreeRanked = exampleThreeExpanded.getRanked();

  BOOST_CHECK_MESSAGE(
    (exampleThreeRanked == std::vector<
      std::vector<unsigned long>
    > { {6}, {3}, {2}, {1} }),
    "Example three expanded is not {{6}, {3}, {2}, {1}}, but: "
    << temple::condenseIterable(
      temple::map(
        exampleThreeRanked,
        [](const auto& set) -> std::string {
          return temple::condenseIterable(set);
        }
      )
    )
  );

  auto exampleThreeExpandedAgain = RankingTree(
    exampleThree.getGraph(),
    exampleThree.getCycleData(),
    exampleThree.getStereocenterList(),
    exampleThree.dumpGraphviz(),
    1,
    {},
    RankingTree::ExpansionOption::Full
  );

  BOOST_CHECK((
    exampleThreeExpandedAgain.getRanked() == std::vector<
      std::vector<unsigned long>
    > { {7}, {4}, {2}, {0} }
  ));
}

template<typename T>
std::string condenseSets(const std::vector<std::vector<T>>& sets) {
  return temple::condenseIterable(
    temple::map(
      sets,
      [](const auto& set) -> std::string {
        return "{"s + temple::condenseIterable(set) + "}"s;
      }
    )
  );
}

BOOST_AUTO_TEST_CASE(sequenceRuleThreeTests) {
  // P-92.4.2.1 Example 1 (Z before E)
  auto ZEDifference = IO::read(
    directoryPrefix + "2Z5S7E-nona-2,7-dien-5-ol.mol"s
  );

  BOOST_CHECK_MESSAGE(
    isStereocenter(ZEDifference, 0, 2, 0),
    "Stereocenter at C0 in 2Z5S7E-nona-2,7-dien-5-ol is not S"
  );

  // P-92.4.2.2 Example 1 (Z before E in aux. stereocenters, splitting)
  auto EECyclobutane = IO::read(
    directoryPrefix + "1E3E-1,3-difluoromethylidenecyclobutane.mol"
  );

  BOOST_CHECK_MESSAGE(
    isStereocenter(EECyclobutane, EECyclobutane.edge(0, 3), 2, 0)
    && isStereocenter(EECyclobutane, EECyclobutane.edge(5, 6), 2, 0),
    "1E3E-1,3-difluoromethylidenecyclobutane double bonds aren't E"
  );

  // P-92.4.2.2 Example 2 (stereogenic before non-stereogenic)
  auto inTreeNstgDB = IO::read(
    directoryPrefix
    + "(2Z5Z7R8Z11Z)-9-(2Z-but-2-en-1-yl)-5-(2E-but-2-en-1-yl)trideca-2,5,8,11-tetraen-7-ol.mol"s
  );

  BOOST_CHECK_MESSAGE(
    isStereocenter(inTreeNstgDB, 0, 2, 1u),
    "(2Z5Z7R8Z11Z)-9-(2Z-but-2-en-1-yl)-5-(2E-but-2-en-1-yl)trideca-2,5,8,11-tetraen-7-ol "
    "difference between non-stereogenic auxiliary stereocenter and assigned "
    "stereocenter isn't recognized!"
  );
}

BOOST_AUTO_TEST_CASE(sequenceRuleFourTests) {
  // (4A) P-92.5.1 Example (stereogenic before non-stereogenic)
  auto pseudoOverNonstg = IO::read(
    directoryPrefix
    + "(2R,3s,4S,6R)-2,6-dichloro-5-(1R-1-chloroethyl)-3-(1S-1-chloroethyl)heptan-4-ol.mol"s
  );

  BOOST_CHECK_MESSAGE(
    !isStereogenic(pseudoOverNonstg, 10),
    "(2R,3s,4S,6R)-2,6-dichloro-5-(1R-1-chloroethyl)-3-(1S-1-chloroethyl)heptan-4-ol.mol "
    "branch with R-R aux. stereocenters not non-stereogenic"
  );

  BOOST_CHECK_MESSAGE(
    isStereogenic(pseudoOverNonstg, 1),
    "(2R,3s,4S,6R)-2,6-dichloro-5-(1R-1-chloroethyl)-3-(1S-1-chloroethyl)heptan-4-ol.mol "
    "branch with R-S aux. stereocenters not stereogenic"
  );

  BOOST_CHECK_MESSAGE(
    isStereocenter(pseudoOverNonstg, 0, 2, 0),
    "(2R,3s,4S,6R)-2,6-dichloro-5-(1R-1-chloroethyl)-3-(1S-1-chloroethyl)heptan-4-ol.mol "
    "sequence rule 4A does not recognize stereogenic over non-stereogenic, 3 as S"
  );

  // (4B) P-92.5.2.2 Example 1 (single chain pairing, ordering and reference selection)
  auto simpleLikeUnlike = IO::read(
    directoryPrefix + "(2R,3R,4R,5S,6R)-2,3,4,5,6-pentachloroheptanedioic-acid.mol"s
  );

  BOOST_CHECK_MESSAGE(
    isStereocenter(simpleLikeUnlike, 10, 2, 1u),
    "(2R,3R,4R,5S,6R)-2,3,4,5,6-pentachloroheptanedioic-acid central carbon does "
    " not register as a stereocenter and/or isn't assigned as R"
  );

  // (4B) P-92.5.2.2 Example 3 (single-chain pairing, cycle splitting)
  auto lAlphaLindane = IO::read(
    directoryPrefix + "l-alpha-lindane.mol"s
  );

  BOOST_CHECK_MESSAGE(
    (
      temple::all_of(
        std::vector<AtomIndexType> {6, 7, 8, 9, 10, 11},
        [&](const auto carbonIndex) -> bool {
          return isStereogenic(lAlphaLindane, carbonIndex);
        }
      )
    ),
    "Not all L-alpha-lindane carbon atoms not recognized as stereocenters!"
  );

  // (4B) P-92.5.2.2 Example 4 (multiple-chain stereocenter ranking)
  auto oxyNitroDiffBranches = IO::read(
    directoryPrefix + "(2R,3S,6R,9R,10S)-6-chloro-5-(1R,2S)-1,2-dihydroxypropoxy-7-(1S,2S)-1,2-dihydroxypropoxy-4,8-dioxa-5,7-diazaundecande-2,3,9,10-tetrol.mol"s
  );

  BOOST_CHECK_MESSAGE(
    isStereocenter(oxyNitroDiffBranches, 0, 2, 1u),
    "(2R,3S,6R,9R,10S)-6-chloro-5-(1R,2S)-1,2-dihydroxypropoxy-7-(1S,2S)-1,2-dihydroxypropoxy-4,8-dioxa-5,7-diazaundecande-2,3,9,10-tetrol central carbon not recognized as R"
  );

  // (4B) P-92.5.2.2 Example 5 (multiple-chain stereocenter ranking)
  auto groupingDifferences = IO::read(
    directoryPrefix + "(2R,3R,5R,7R,8R)-4.4-bis(2S,3R-3-chlorobutan-2-yl)-6,6-bis(2S,4S-3-chlorobutan-2-yl)-2,8-dichloro-3,7-dimethylnonan-5-ol.mol"s
  );

  BOOST_CHECK_MESSAGE(
    isStereocenter(groupingDifferences, 0, 2, 1u),
    "The central carbon in (2R,3R,5R,7R,8R)-4.4-bis(2S,3R-3-chlorobutan-2-yl)-6,6-bis(2S,4S-3-chlorobutan-2-yl)-2,8-dichloro-3,7-dimethylnonan-5-ol is not recognized as R"
  );

  // (4B) P-92.5.2.2 Example 6 (number of reference descriptors)
  auto numReferenceDescriptors = IO::read(
    directoryPrefix + "2R-2-bis(1R)-1-hydroxyethylamino-2-(1R)-1-hydroxyethyl(1S)-1-hydroxyethylaminoacetic-acid.mol"
  );

  BOOST_CHECK_MESSAGE(
    isStereocenter(numReferenceDescriptors, 0, 2, 1u),
    "The central carbon in 2R-2-bis(1R)-1-hydroxyethylamino-2-(1R)-1-hydroxyethyl(1S)-1-hydroxyethylaminoacetic-acid is not recognized as R"
  );
}

BOOST_AUTO_TEST_CASE(sequenceRuleFiveTests) {
  // (4C) P-92.5.3 Example r/s leads to R difference
  auto rsDifference = IO::read(
    directoryPrefix + "(2R,3r,4R,5s,6R)-2,6-dichloro-3,5-bis(1S-1-chloroethyl)heptan-4-ol.mol"
  );

  BOOST_CHECK_MESSAGE(
    isStereocenter(rsDifference, 0, 2, 1u),
    "The central carbon in (2R,3r,4R,5s,6R)-2,6-dichloro-3,5-bis(1S-1-chloroethyl)heptan-4-ol is not recognized as R"
  );

  // (5) P-92.6 Example 1 simple R/S difference leads to r
  auto pseudo = IO::read(
    directoryPrefix + "(2R,3r,4S)-pentane-2,3,4-trithiol.mol"
  );

  BOOST_CHECK_MESSAGE(
    isStereocenter(pseudo, 0, 2, 1u),
    "The central carbon in (2R,3r,4S)-pentane-2,3,4-trithiol is not recognized as R"
  );

  // (5) P-92.6 Example 2 cyclobutane splitting
  auto cyclobutane = IO::read(
    directoryPrefix + "(1r,3r)-cyclobutane-1,3-diol.mol"
  );

  BOOST_CHECK_MESSAGE(
    isStereocenter(cyclobutane, 2, 2, 1u)
    && isStereocenter(cyclobutane, 3, 2, 1u),
    "The chiral carbons in (1r,3r)-cyclobutane-1,3-diol aren't properly recognized"
  );

  // (5) P-92.6 Example 5 double bond ranking
  auto pseudoDB = IO::read(
    directoryPrefix + "(2E,4R)-4-chloro-3-(1S-1-chloroethyl)pent-2-ene.mol"
  );

  BOOST_CHECK_MESSAGE(
    isStereocenter(pseudoDB, pseudoDB.edge(0, 3), 2, 0u),
    "Double bond in (2E,4R)-4-chloro-3-(1S-1-chloroethyl)pent-2-ene isn't E"
  );

  // (5) P-92.6 Example 6
  auto fourDoesNothing = IO::read(
    directoryPrefix + "1s-1-(1R,2R-1,2-dichloropropyl-1S,2R-1,2-dichloropropylamino)1-(1R,2S-1,2-dichloropropyl-1S,2S-1,2-dichloropropylamino)methan-1-ol.mol"
  );

  BOOST_CHECK_MESSAGE(
    isStereocenter(fourDoesNothing, 0, 2, 0u),
    "The central stereocenter in 1s-1-(1R,2R-1,2-dichloropropyl-1S,2R-1,2-dichloropropylamino)1-(1R,2S-1,2-dichloropropyl-1S,2S-1,2-dichloropropylamino)methan-1-ol isn't recognized as S"
  );
}

const std::unordered_map<unsigned, std::string> cipIdentifiers {
  {100, "2Z 3Z 4E 5E"},
  {101, "1Z 3E 6Z 8E"},
  {102, "2R 3R 6S 12S"},
  {103, "2R 3R 5R 6R"},
  {104, "1R"},
  {105, "2S 3R 4S 5S 6R"},
  {106, "3S 4R"},
  {107, "1Z 2Z 3E 4E 5E 12E"},
  {108, "1R 2R"},
  {109, "2R"},
  {10, "19P 20P"},
  {110, "13E 14E"},
  {111, "1S 3S 5S 6R"},
  {112, "1R 2R"},
  {113, "2R 4S"},
  {114, "2Z 3Z"},
  {115, "2S 3R 4R 5S 6R"},
  {116, "1R 3R"},
  {117, "1Z 2Z"},
  {118, "1E 4E"},
  {119, "2R 3R"},
  {11, "19M 20M"},
  {120, "1R 27P 28P 42R"},
  {121, "7E 8E"},
  {122, "1S"},
  {123, "1S"},
  {124, "17S 20R"},
  {125, "10E 11E 13S 15S"},
  {126, "2E 3Z 4Z 5E 20R 21S 24R 26S 27R 30S 39E 40E 43E 44E"},
  {127, "2E 3E 4E 5E 20S 21R 24S 26R 27S 30R 39E 40E 43E 44E"},
  {128, "2S 8S 12S"},
  {129, "2R 8R 12R"},
  {130, "1S"},
  {131, "1S 4R"},
  {132, "2S 6S"},
  {133, "6S"},
  {134, "2R"},
  {135, "3Z 5Z"},
  {136, "7R 11S"},
  {137, "1Z 2Z"},
  {138, "7R"},
  {139, "3R"},
  {13, "1Z 2E 6E 7Z"},
  {140, "2E 5E"},
  {141, "1P 5P 6Z 7Z"},
  {142, "1S"},
  {143, "5S 6S 10S"},
  {144, "2P 4P"},
  {145, "3R"},
  {146, "5S 6Z 7Z 8E 9E 10E 11E 12R 14Z 15Z"},
  {147, "1S"},
  {148, "1E 2E"},
  {149, "2R"},
  {14, "10R"},
  {150, "5R 6S"},
  {151, "2R"},
  {152, "2R 5R 8S 9R"},
  {153, "1Z 2Z 4E 5E"},
  {154, "3E 5E 7E 9E"},
  {155, "7Z 8Z 12Z 13Z"},
  {156, "1R"},
  {157, "2E 3E 4E 5E 18R 23S"},
  {158, "1P 7P"},
  {159, "1R 3R 5S 6S"},
  {15, "2R"},
  {160, "2S 5S 7S"},
  {161, "2Z 3Z 6S"},
  {162, "1R"},
  {163, "2S 3S"},
  {164, "27Z 28Z 40R"},
  {165, "1R"},
  {166, "3P 6P"},
  {167, "2S"},
  {168, "1S 2R"},
  {169, "1R 2R"},
  {16, "2R 4R"},
  {170, "1S"},
  {171, "1S"},
  {172, "2R 3S"},
  {173, "1S 3Z 4Z 17Z 18Z 22Z 23Z"},
  {174, "5R 7R"},
  {175, "2R"},
  {176, "2Z 5Z"},
  {177, "1S"},
  {178, "2S"},
  {179, "1S"},
  {17, "2S 4S"},
  {180, "1S"},
  {181, "1R"},
  {182, "1R"},
  {183, "1R"},
  {184, "1E 2E"},
  {185, "2Z 3Z"},
  {186, "1S"},
  {187, "7S"},
  {188, "1E 3E 5E 7E"},
  {189, "1Z 4Z 7Z 9Z"},
  {18, "1S 4E 5E 12R"},
  {190, "3E 4E 5E 6E 9E 10E"},
  {191, "2Z 3Z 5Z 6Z 7S 11Z 12Z 16Z 17Z 20E 21E"},
  {192, "2Z 3Z 7R 11E 12E"},
  {193, "2Z 3Z 7S 11E 12E"},
  {194, "2Z 3Z 7R 11E 12E"},
  {195, "2Z 3Z 5R 7E 8E"},
  {196, "1R 2S 3S 4R 5S 6R"},
  {197, "1R 2R 3R 4R 5S 6S"},
  {198, "2R 4S 5S 8S 10S 13R 15R 20S 21R 23S 25R 28R 30R"},
  {199, "2R 3R 4R 5R 6S"},
  {19, "5E 7E"},
  {200, "2R 3S 4S 5R 6R"},
  {201, "2R 3R 4R 5S 6R"},
  {202, "1R 7R 8S 9R 10R"},
  {203, "2R 3S 6R 9S 10S 18R 19S 24R 25S"},
  {204, "2S 3R 6S 9S 10R 13S 14S 17R 18S"},
  {205, "1s 6s 9r 10r 17s 18s"},
  {206, "1s 6s 9s 10s"},
  {207, "1r 2s 6s 11r 15r 22r"},
  {208, "9r 10r 19r 28r"},
  {209, "1r 2r 3r 4r 5r 6r"},
  {20, "2Z 3Z"},
  {210, "1s 2s 3s 4s 5s 6s"},
  {211, "1s 2s"},
  {212, "1r 2r"},
  {213, "1r 4r"},
  {214, "1E 6s 7E 12s"},
  {215, "2s 17s"},
  {216, "2s 17s"},
  {217, "2s 17s"},
  {218, "21s 23s"},
  {219, "1r 3r"},
  {21, "2R"},
  {220, "1s 6r 9s 10s"},
  {221, "1r 4r"},
  {222, "1s 4s"},
  {223, "9r 10r 19r 28r"},
  {224, "11s 12r 15s 20r"},
  {225, "1s 9s"},
  {226, "7r 10r 15s 18s 27r 30r 31s 34s 39r 42r"},
  {227, "2R 3r 4S"},
  {228, "2R 3s 4S"},
  {229, "1e 2e 4R 5S"},
  {22, "1S"},
  {230, "1R 2S 4r"},
  {231, "1p 6s 8p"},
  {232, "2m 4m 7m 11m"},
  {233, "1r 3R 7S 11R 16S 20S 27r 32S 37R 41R 44r 46S 50R 54R 58S"},
  {234, "6R 8s 16S"},
  {235, "1S 2r 4R 12S"},
  {236, "2S 3r 4R"},
  {237, "2R 5r 8S"},
  {238, "1S 4R 5s"},
  {239, "1s 5R 6S 7R 8S 9S 10S 11R 12R"},
  {23, "2M 3M"},
  {240, "3E 4E 6s 9E 10E 13s"},
  {241, "1R 2s 4S"},
  {242, "2R 5R 8r 10S 13S 20s 22S 25R 29R 32S 36R 42r 44S 47R 51s 53R 56R 60S 63S 67R 70S"},
  {243, "2m 3R 4S 5m"},
  {244, "2S 4S 5R 10R 15S"},
  {245, "1E 2E 3S 7R 11S 15R"},
  {246, "2z 3z 6R"},
  {247, "1z 6R 7z"},
  {248, "3z 4S 5z"},
  {249, "1r 3R 7S 10R 12R 17S 21S 25R 28s 30S 32S 34S 39R 43R 46r 48S 52R 56R 60S"},
  {24, "1S 3S 5R 7R"},
  {250, "1r 3R 7S 10R 12R 17S 21S 25S 28r 30R 32S 34S 39R 43R 46r 48S 52R 56R 60S"},
  {251, "2R 3s 4S 6R 11S 12R"},
  {252, "1s 2r 3S 4R"},
  {253, "1R 2s 3S 4S 5s 6R"},
  {254, "1S 2r 3R 4S 5r 6R"},
  {255, "1R 2s 3S 4S 5r 6R"},
  {256, "1s 2r 3S 4R"},
  {257, "1S 3r 6S"},
  {258, "1R 3r 6R"},
  {259, "1S 8r 9S 16E 17E 18r 19S"},
  {25, "6R"},
  {260, "1s 6R 9S 10S 14S"},
  {261, "2R 4R 7r 9S 12S 16S 19s 21S 24R 28R 31S 35S 38r 40S 43R 47s 49R 52R 56S 59S 63R 66S 111R 112S 122s"},
  {262, "1r 3R 7S 11R 16R 20S 27R 32S 37R 41R 44r 46S 50R 54R 58R"},
  {263, "2R 4R 7r 9S 12S 16S 19s 21S 24R 28R 31S 35S 38r 40S 43R 47s 49R 52R 56S 59S 63R 66S 111R 112S 122s"},
  {264, "2R 4R 7r 9S 12S 19s 21S 24R 28R 31S 38r 40S 43R 47s 49R 52R 56S 59S 63R 66S 111R 112S 122s"},
  {265, "2R 4R 7r 9S 12S 16S 19s 21S 24R 28R 31S 35S 38r 40S 43R 47s 49R 52R 56S 59S 63R 66S 111R 122S"},
  {266, "2R 4R 7r 9S 12S 19s 21S 24R 28R 31S 38r 40S 43R 47s 49R 52R 56S 59S 63R 66S 111R 122S"},
  {267, "2R 5R 8r 10S 13S 21r 26r 28R 31S 36s 38S 41S 45S 48R 53s 55R 58S 62S 65R 69R 72R 88R 89S"},
  {268, "2R 5R 8r 10S 13S 18S 21S 24R 26r 28R 31S 36s 38S 41S 45S 48R 53s 55R 58S 62S 65R 69R 72R 88R 89S"},
  {269, "1r 3R 7S 10R 12R 17S 21S 25R 28S 30R 32S 34S 39R 43R 46r 48S 52R 56R 60S"},
  {26, "3Z 4R 5Z 7E 9S 10E"},
  {270, "1r 3R 7S 10R 12R 17S 21S 25S 28R 30S 32S 34S 39R 43R 46r 48S 52R 56R 60S"},
  {271, "13S 14S 15R 16S 17R 18R 19S 20S 22r 23S 24S"},
  {272, "1Z 2Z 3R 5r 6R 10S 15S 19S 23S 27R 30Z 31Z 32R 35S 39S 44S 47s 48S 52R 56R"},
  {273, "1R 3R 4r 5R 9S 10S 13s 14S 15R 18S 21S 23r 24S 25R 28s 29S 30R 33S"},
  {274, "1R 2s 3S 4r 5R 8s 9R 10S 14S 15S 18S 20s 21S 24R 25R 28r 29R 30S 33R"},
  {275, "1r 3R 7S 10s 12R 15r 17R 21S 25R 28S 30R 32s 34R 37s 39S 43S 46s 48R 52R 56S 60S"},
  {276, "2R 3s 4R 5r 6R 11S 12S"},
  {277, "1r 3R 7S 10R 12R 17S 21S 25S 28S 30R 32S 34S 39R 43R 46s 48R 52R 56S 60S"},
  {278, "1r 3R 7S 10s 12R 15r 17R 21S 25R 28R 30R 32S 34R 37r 39R 43S 46s 48R 52S 56S 60S"},
  {279, "2R 5r 8S 12s 15S"},
  {27, "2R 6S"},
  {280, "3R 7R 11R 15R 19R"},
  {281, "5S"},
  {282, "5R"},
  {283, "10S"},
  {284, "10R"},
  {285, "2R"},
  {286, "2S"},
  {287, "1M 3M"},
  {288, "4S"},
  {289, "2R"},
  {28, "1S 5R 7S"},
  {290, "1S"},
  {291, "2R"},
  {292, "5S"},
  {293, "3Z 4Z 7Z 8Z 11Z 12Z 14S"},
  {294, "2R 3S 5R 8R 9S"},
  {295, "2S 3S 5R 8S 9R"},
  {296, "1S 9S 14S"},
  {297, "1R 6S 11S"},
  {298, "1R 2S 5R"},
  {299, "2S 4z 6z 9z 17z"},
  {29, "2S 3S 4R"},
  {300, "10S 39R 61s"},
  {30, "2R 3R 4S"},
  {31, "1R"},
  {32, "2S 3Z 4Z"},
  {33, "2S 3Z 4Z"},
  {34, "1R 2S 4S"},
  {35, "2R 3R 8S"},
  {36, "1Z 2Z"},
  {37, "5R"},
  {38, "5R"},
  {39, "5S"},
  {40, "5R 19S 20S 24R 27R"},
  {41, "9E 10E"},
  {42, "8E 9E"},
  {43, "2R 3R"},
  {44, "5E 6E"},
  {45, "1S"},
  {46, "1R 2R"},
  {47, "4R 5S"},
  {48, "4S 5S"},
  {49, "1E 2E 3E 4Z 9E 11Z"},
  {50, "3S 4S 5R 6R"},
  {51, "8R 19R 23R"},
  {52, "1R 5R 6S"},
  {53, "2E 3E 4E 5E 10R 14E 15E 16E 17E"},
  {54, "9S"},
  {55, "2P 11P"},
  {56, "6S 7S"},
  {57, "1M 7M"},
  {58, "3Z 4Z 5E 6E"},
  {59, "1R"},
  {60, "1Z 2Z"},
  {61, "2E 3E"},
  {62, "7Z 8Z"},
  {63, "1S 27E 28E 44R"},
  {64, "4R"},
  {65, "3R 4R 5S 6R"},
  {66, "2S 3S"},
  {67, "2R 3R"},
  {68, "2R"},
  {69, "1S 4R"},
  {70, "3R 4R 5S 10S 13S 17S 18R 21S"},
  {71, "1S"},
  {72, "1M 7M 21S"},
  {73, "1M 7M"},
  {74, "1S"},
  {75, "1R 2S 5S 6S 8R"},
  {76, "1R"},
  {77, "1S"},
  {78, "27P 28P 44R"},
  {79, "2M 3M"},
  {80, "1R 2S"},
  {81, "9S"},
  {82, "4R 5R"},
  {83, "1S"},
  {84, "1R 3R 4S 7S"},
  {85, "1E 11E"},
  {86, "1P 7P"},
  {87, "2E 3E 4E 5E 18S 23S"},
  {88, "2Z 3E 4E 5Z 18S 23S"},
  {89, "4R"},
  {90, "2R"},
  {91, "4S 5R 6S 7S 8S 13R"},
  {92, "1R"},
  {93, "5E 6E 8Z 9Z"},
  {94, "7R"},
  {95, "8S"},
  {96, "1Z 3Z"},
  {97, "15R"},
  {98, "1R 2S 5R"},
  {99, "1R"}
};

enum class StereocenterType {A, B};

const std::map<char, std::pair<StereocenterType, unsigned>> descriptorToPermutationMap {
  {'R', {StereocenterType::A, 1}},
  {'S', {StereocenterType::A, 0}},
  {'r', {StereocenterType::A, 1}},
  {'s', {StereocenterType::A, 0}},
  {'E', {StereocenterType::B, 1}},
  {'Z', {StereocenterType::B, 0}}
};

struct Stereodescriptor {
  std::size_t atomIndex;
  StereocenterType type;
  unsigned permutation;

  bool operator < (const Stereodescriptor& other) const {
    return (
      std::tie(atomIndex, type, permutation)
      < std::tie(other.atomIndex, other.type, other.permutation)
    );
  }
};

std::ostream& operator << (std::ostream& os, const Stereodescriptor& descriptor) {
  os << descriptor.atomIndex
    << "-" << (descriptor.type == StereocenterType::A ? 'A' : 'B')
    << "-" << descriptor.permutation;
  return os;
}

std::set<Stereodescriptor> makeDescriptorSet(const Molecule& molecule) {
  std::set<Stereodescriptor> descriptors;

  for(const auto& atomStereocenter : molecule.getStereocenterList().atomStereocenters()) {
    if(atomStereocenter.numStereopermutations() > 1 && atomStereocenter.assigned()) {
      Stereodescriptor descriptor;
      descriptor.atomIndex = atomStereocenter.centralIndex();
      descriptor.type = StereocenterType::A;
      descriptor.permutation = atomStereocenter.indexOfPermutation().value();

      descriptors.insert(descriptor);
    }
  }

  for(const auto& bondStereocenter : molecule.getStereocenterList().bondStereocenters()) {
    if(bondStereocenter.numStereopermutations() > 1 && bondStereocenter.assigned()) {
      Stereodescriptor descriptor;
      descriptor.type = StereocenterType::B;
      descriptor.permutation = bondStereocenter.indexOfPermutation().value();

      auto vertices = molecule.vertices(bondStereocenter.edge());
      descriptor.atomIndex = vertices.front();
      descriptors.insert(descriptor);
      descriptor.atomIndex = vertices.back();
      descriptors.insert(descriptor);
    }
  }

  return descriptors;
}

enum class TestFlags : unsigned {
  Skipped,
  Unexpected,
  MissingExpected
};

BOOST_AUTO_TEST_CASE(CIPValidationSuiteTests) {
  /* Test against a validation suite of organic molecules
   *
   * Reference:
   *
   * Algorithmic Analysis of Cahn–Ingold–Prelog Rules of Stereochemistry:
   * Proposals for Revised Rules and a Guide for Machine Implementation
   *
   * Robert M. Hanson, Sophia Musacchio, John W. Mayfield, Mikko J. Vainio,
   * Andrey Yerin, Dmitry Redkin
   *
   * J. Chem. Inf. Model.
   * DOI: 10.1021/acs.jcim.8b00324
   */
  boost::filesystem::path filesPath("test_files/cip_validation");
  boost::filesystem::recursive_directory_iterator end;

  std::vector<temple::Bitmask<TestFlags>> summaries;
  summaries.reserve(310);

  for(boost::filesystem::recursive_directory_iterator i(filesPath); i != end; i++) {
    const boost::filesystem::path currentFilePath = *i;

    if(currentFilePath.extension() != ".mol") {
      // Do not add a summary for this
      continue;
    }

    Molecule molecule;

    // Some IO will fail due to unsanitized disconnected ions
    try {
      molecule = IO::read(currentFilePath.string());
    } catch(const std::exception& e) {
      BOOST_ERROR(
        "Exception in IO for " << currentFilePath.string()
        << ": " << e.what() << "\n"
      );

      // Skip this molecule
      summaries.push_back(
        temple::make_bitmask(TestFlags::Skipped)
      );
      continue;
    }

    auto stem = currentFilePath.stem().string();
    auto findNumeric = std::find_if(
      std::begin(stem),
      std::end(stem),
      [](unsigned char character) -> bool {
        return static_cast<bool>(std::isdigit(character));
      }
    );

    unsigned structureNumber = std::stoul(stem.substr(findNumeric - std::begin(stem)));
    auto findIter = cipIdentifiers.find(structureNumber);

    bool skipMolecule = false;

    std::set<Stereodescriptor> expectedDescriptors;
    if(findIter != std::end(cipIdentifiers)) {
      for(
        const auto& stereodescriptorString :
        StdlibTypeAlgorithms::split(findIter->second, ' ')
      ) {
        auto mapInfoIter = descriptorToPermutationMap.find(stereodescriptorString.back());
        if(mapInfoIter == std::end(descriptorToPermutationMap)) {
          /* If any descriptors are present that we do not handle, we have to
           * skip the structure entirely
           */
          skipMolecule = true;
          break;
        }

        Stereodescriptor descriptor;
        descriptor.atomIndex = std::stoul(
          stereodescriptorString.substr(0, stereodescriptorString.size() - 1)
        );

        /* Index origin is 1 for their CIP descriptors, so we need to decrement
         * to reach our indexing scheme
         */
        --descriptor.atomIndex;
        std::tie(descriptor.type, descriptor.permutation) = mapInfoIter->second;

        expectedDescriptors.insert(descriptor);
      }
    }

    if(skipMolecule) {
      summaries.push_back(
        temple::make_bitmask(TestFlags::Skipped)
      );
      continue;
    }

    auto foundDescriptors = makeDescriptorSet(molecule);

    bool pass = true;
    std::ostringstream failureMessages;

    std::set<Stereodescriptor> discrepancies;

    std::set_difference(
      std::begin(expectedDescriptors),
      std::end(expectedDescriptors),
      std::begin(foundDescriptors),
      std::end(foundDescriptors),
      std::inserter(discrepancies, discrepancies.end())
    );

    auto summary = temple::Bitmask<TestFlags> {};

    if(!discrepancies.empty()) {
      pass = false;
      summary |= TestFlags::MissingExpected;

      failureMessages << stem << " does not match validation set:\n";
      failureMessages << "- Expected but not found: ";
      for(const auto& discrepancy : discrepancies) {
        failureMessages << discrepancy << ", ";
      }
      failureMessages << "\n";
    }

    discrepancies.clear();

    std::set_difference(
      std::begin(foundDescriptors),
      std::end(foundDescriptors),
      std::begin(expectedDescriptors),
      std::end(expectedDescriptors),
      std::inserter(discrepancies, discrepancies.end())
    );

    if(!discrepancies.empty()) {
      summary |= TestFlags::Unexpected;

      if(pass) {
        failureMessages << stem << " does not match validation set:\n";
      }
      // Do not fail it for adding unexpected ones as long as the expected ones are unchanged and not missing
      //pass = false;
      failureMessages << "- Found but not expected: ";
      for(const auto& discrepancy : discrepancies) {
        failureMessages << discrepancy << ", ";
      }
      failureMessages << "\n";
    }

    if(!pass) {
      discrepancies.clear();
      std::set_intersection(
        std::begin(foundDescriptors),
        std::end(foundDescriptors),
        std::begin(expectedDescriptors),
        std::end(expectedDescriptors),
        std::inserter(discrepancies, discrepancies.end())
      );

      failureMessages << "- Correct: ";
      for(const auto& discrepancy : discrepancies) {
        failureMessages << discrepancy << ", ";
      }
      failureMessages << "\n";

      std::cout << failureMessages.str();
    }

    summaries.push_back(summary);
    BOOST_CHECK(pass);
  }

  // Summarize the results
  auto skipped = std::accumulate(
    std::begin(summaries),
    std::end(summaries),
    0u,
    [](const unsigned carry, const temple::Bitmask<TestFlags>& flags) -> unsigned {
      if(flags.isSet(TestFlags::Skipped)) {
        return carry + 1;
      }

      return carry;
    }
  );

  auto errors = std::accumulate(
    std::begin(summaries),
    std::end(summaries),
    0u,
    [](const unsigned carry, const temple::Bitmask<TestFlags>& flags) -> unsigned {
      if(flags.isSet(TestFlags::MissingExpected)) {
        return carry + 1;
      }

      return carry;
    }
  );

  auto withUnexpected = std::accumulate(
    std::begin(summaries),
    std::end(summaries),
    0u,
    [](const unsigned carry, const temple::Bitmask<TestFlags>& flags) -> unsigned {
      if(flags.isSet(TestFlags::Unexpected)) {
        return carry + 1;
      }

      return carry;
    }
  );

  unsigned total = summaries.size();

  std::cout << "\nTotal: " << total << "\n"
    << "Skipped: " << skipped << " (" << std::round(100.0 * skipped / total) << " %)\n"
    << "Missing expected stereocenters (failures): " << errors << " (" << std::round(100.0 * errors / (total - skipped)) << " %)\n"
    << "With unexpected stereocenters: " << withUnexpected << " (" << std::round(100.0 * withUnexpected / (total - skipped)) << " %)\n";
}
