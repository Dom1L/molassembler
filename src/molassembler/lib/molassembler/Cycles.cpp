/*!@file
 * @copyright ETH Zurich, Laboratory for Physical Chemistry, Reiher Group.
 *   See LICENSE.txt
 */

#include "molassembler/Cycles.h"

#include "boost/range/iterator_range_core.hpp"

#include "molassembler/Detail/StdlibTypeAlgorithms.h"
#include "molassembler/Graph/InnerGraph.h"

#include "temple/TinySet.h"

namespace Scine {

namespace molassembler {

/* Cycles member Type declarations */
struct Cycles::RDLDataPtrs {
  //! Raw pointers to RDL_graph object
  RDL_graph* graphPtr;

  //! Raw pointer to calculated graph cycle data
  RDL_data* dataPtr;

  RDLDataPtrs() = delete;
  RDLDataPtrs(const InnerGraph& sourceGraph, bool ignoreEtaBonds);

  RDLDataPtrs(const RDLDataPtrs& other) = delete;
  RDLDataPtrs(RDLDataPtrs&& other) = delete;
  RDLDataPtrs& operator = (const RDLDataPtrs& other) = delete;
  RDLDataPtrs& operator = (RDLDataPtrs&& other) = delete;
  ~RDLDataPtrs();
};

struct Cycles::constIterator::RDLCyclePtrs {
  RDL_cycleIterator* cycleIterPtr;
  RDL_cycle* cyclePtr;

  RDLCyclePtrs() = delete;
  RDLCyclePtrs(const RDLDataPtrs& dataPtrs);

  RDLCyclePtrs(const RDLCyclePtrs& other) = delete;
  RDLCyclePtrs(RDLCyclePtrs&& other) = delete;
  RDLCyclePtrs& operator = (const RDLCyclePtrs& other) = delete;
  RDLCyclePtrs& operator = (RDLCyclePtrs&& other) = delete;
  ~RDLCyclePtrs();

  /*! Advance internal iterator and cycle state
   *
   * Frees the memory for the current cycle and advances the iterator state.
   * If the iterator is now not at the end of all relevant cycles, then
   * the next cycle is allocated and constructed into cyclePtr. Otherwise,
   * cyclePtr is a nullptr.
   */
  void advance();
};

Cycles::Cycles(
  const OuterGraph& sourceGraph,
  const bool ignoreEtaBonds
) {
  _rdlPtr = std::make_shared<RDLDataPtrs>(sourceGraph.inner(), ignoreEtaBonds);
}

Cycles::Cycles(
  const InnerGraph& sourceGraph,
  const bool ignoreEtaBonds
) {
  _rdlPtr = std::make_shared<RDLDataPtrs>(sourceGraph, ignoreEtaBonds);
}

unsigned Cycles::numCycleFamilies() const {
  return RDL_getNofURF(_rdlPtr->dataPtr);
}

unsigned Cycles::numCycleFamilies(const AtomIndex index) const {
  return RDL_getNofURFContainingNode(_rdlPtr->dataPtr, index);
}

//! Returns the number of relevant cycles (RCs)
unsigned Cycles::numRelevantCycles() const {
  return RDL_getNofRC(_rdlPtr->dataPtr);
}

unsigned Cycles::numRelevantCycles(const AtomIndex index) const {
  return RDL_getNofRCFContainingNode(_rdlPtr->dataPtr, index);
}

Cycles::constIterator Cycles::begin() const {
  return {_rdlPtr};
}

Cycles::constIterator Cycles::end() const {
  return {
    _rdlPtr,
    predicates::All {},
    numRelevantCycles()
  };
}

RDL_data* Cycles::dataPtr() const {
  return _rdlPtr->dataPtr;
}

bool Cycles::operator == (const Cycles& other) const {
  return _rdlPtr == other._rdlPtr;
}

bool Cycles::operator != (const Cycles& other) const {
  return !(*this == other);
}

/* Cycles::RDLDataPtrs */
Cycles::RDLDataPtrs::RDLDataPtrs(
  const InnerGraph& sourceGraph,
  const bool ignoreEtaBonds
) {
  // Initialize a new graph
  graphPtr = RDL_initNewGraph(sourceGraph.N());

  if(ignoreEtaBonds) {
    for(const auto edge : boost::make_iterator_range(sourceGraph.edges())) {
      // Copy vertices (without bond type information)
      if(sourceGraph.bondType(edge) != BondType::Eta) {
        auto edgeAddResult = RDL_addUEdge(
          graphPtr,
          sourceGraph.source(edge),
          sourceGraph.target(edge)
        );

        if(edgeAddResult == RDL_INVALID_RESULT || edgeAddResult == RDL_DUPLICATE_EDGE) {
          throw std::runtime_error("RDL add edge failed!");
        }
      }
    }
  } else {
    for(const auto edge : boost::make_iterator_range(sourceGraph.edges())) {
      auto edgeAddResult = RDL_addUEdge(
        graphPtr,
        sourceGraph.source(edge),
        sourceGraph.target(edge)
      );

      if(edgeAddResult == RDL_INVALID_RESULT || edgeAddResult == RDL_DUPLICATE_EDGE) {
        throw std::runtime_error("RDL add edge failed!");
      }
    }
  }

  // Calculate, and assure yourself of success
  dataPtr = RDL_calculate(graphPtr);
  assert(dataPtr != nullptr);
}

Cycles::RDLDataPtrs::~RDLDataPtrs() {
  // Awfully enough, calling this frees both dataPtr and graphPtr
  RDL_deleteData(dataPtr);
}

/* predicates */
bool Cycles::predicates::All::operator() (const RDL_cycle* const /* cyclePtr */) const {
  return true;
}

Cycles::predicates::SizeLessThan::SizeLessThan(unsigned passThreshold) : threshold(passThreshold) {}

bool Cycles::predicates::SizeLessThan::operator() (const RDL_cycle* const cyclePtr) const {
  return (cyclePtr -> weight) < threshold;
}

Cycles::predicates::ContainsIndex::ContainsIndex(AtomIndex passSoughtIndex) : soughtIndex(passSoughtIndex) {}

bool Cycles::predicates::ContainsIndex::operator() (const RDL_cycle* const cyclePtr) const {
  for(unsigned i = 0; i < cyclePtr->weight; ++i) {
    if(
        cyclePtr -> edges[i][0] == soughtIndex
        || cyclePtr -> edges[i][1] == soughtIndex
      ) {
      return true;
    }
  }

  return false;
}

bool Cycles::predicates::ConsistsOf::operator() (const RDL_cycle* const cyclePtr) const {
  if(cyclePtr->weight != indices.size()) {
    return false;
  }

  for(unsigned i = 0; i < cyclePtr->weight; ++i) {
    if(
      temple::TinySet<AtomIndex>::binary_search(indices, cyclePtr->edges[i][0])
      || temple::TinySet<AtomIndex>::binary_search(indices, cyclePtr->edges[i][1])
    ) {
      return false;
    }
  }

  return true;
}

void Cycles::predicates::ConsistsOf::insert(const AtomIndex i) {
  temple::TinySet<AtomIndex>::checked_insert(indices, i);
}

/* Cycles::constIterator */
Cycles::constIterator::constIterator(
  const std::shared_ptr<RDLDataPtrs>& dataPtr,
  Cycles::constIterator::PredicateType cyclePredicate,
  unsigned rCycleIndex
) : _rdlPtr {dataPtr} {
  _cyclePtr = std::make_shared<RDLCyclePtrs>(*dataPtr);
  _cyclePermissiblePredicate = std::move(cyclePredicate);

  if(rCycleIndex == 0) { // begin constructor
    // In case first cycle is not permissible, advance to first permissible
    if(
      RDL_cycleIteratorAtEnd(_cyclePtr->cycleIterPtr) == 0
      && !_cyclePermissiblePredicate(_cyclePtr->cyclePtr)
    ) {
      ++(*this);
    }
  } else { // end constructor, advance to end
    while(_rCycleIndex < rCycleIndex) {
      ++(*this);
    }

    assert(_rCycleIndex == rCycleIndex);
  }
}

Cycles::constIterator& Cycles::constIterator::operator ++ () {
  if(RDL_cycleIteratorAtEnd(_cyclePtr->cycleIterPtr) == 0) {
    do {
      _cyclePtr->advance();
      ++_rCycleIndex;
    } while(
      RDL_cycleIteratorAtEnd(_cyclePtr->cycleIterPtr) == 0
      && !_cyclePermissiblePredicate(_cyclePtr->cyclePtr)
    );
  } else {
    throw std::logic_error("Advancing Cycles::constIterator past end");
  }

  return *this;
}

Cycles::constIterator Cycles::constIterator::operator ++ (int) {
  constIterator retval = *this;
  ++(*this);
  return retval;
}

std::vector<BondIndex> Cycles::constIterator::operator * () const {
  if(_cyclePtr->cyclePtr == nullptr) {
    throw std::range_error("Dereferencing Cycles::constIterator end iterator");
  }

  const unsigned size = _cyclePtr->cyclePtr->weight;

  temple::TinySet<BondIndex> cycleEdges;
  cycleEdges.reserve(size);

  // Collect the pair of atom indices for every edge of this cycle
  for(unsigned i = 0; i < size; ++i) {
    cycleEdges.insert(
      BondIndex {
        _cyclePtr->cyclePtr->edges[i][0],
        _cyclePtr->cyclePtr->edges[i][1]
      }
    );
  }

  return cycleEdges.data;
}

//! Must be constructed from same Cycles base and at same RC
bool Cycles::constIterator::operator == (const Cycles::constIterator& other) const {
  return (
    _rdlPtr == other._rdlPtr
    && _rCycleIndex == other._rCycleIndex
  );
}

bool Cycles::constIterator::operator != (const Cycles::constIterator& other) const {
  return !(*this == other);
}

/* Cycles::constIterator::RDLCyclePtrs implementation */
Cycles::constIterator::RDLCyclePtrs::RDLCyclePtrs(const RDLDataPtrs& dataPtrs) {
  cycleIterPtr = RDL_getRCyclesIterator(dataPtrs.dataPtr);
  if(RDL_cycleIteratorAtEnd(cycleIterPtr) != 0) {
    cyclePtr = nullptr;
  } else {
    cyclePtr = RDL_cycleIteratorGetCycle(cycleIterPtr);
  }
}

Cycles::constIterator::RDLCyclePtrs::~RDLCyclePtrs() {
  if(cyclePtr != nullptr) {
    RDL_deleteCycle(cyclePtr);
  }

  RDL_deleteCycleIterator(cycleIterPtr);
}

void Cycles::constIterator::RDLCyclePtrs::advance() {
  assert(!RDL_cycleIteratorAtEnd(cycleIterPtr));

  RDL_deleteCycle(cyclePtr);
  cyclePtr = nullptr;

  RDL_cycleIteratorNext(cycleIterPtr);

  if(RDL_cycleIteratorAtEnd(cycleIterPtr) == 0) {
    cyclePtr = RDL_cycleIteratorGetCycle(cycleIterPtr);
  }
}

std::unordered_map<AtomIndex, unsigned> makeSmallestCycleMap(const Cycles& cycleData) {
  std::unordered_map<AtomIndex, unsigned> smallestCycle;

  for(const auto cycleEdges : cycleData) {
    const unsigned cycleSize = cycleEdges.size();

    for(const BondIndex& bond : cycleEdges) {
      for(const AtomIndex index: bond) {
        auto findIter = smallestCycle.find(index);

        if(findIter != std::end(smallestCycle)) {
          if(cycleSize < findIter->second) {
            findIter->second = cycleSize;
          }
        } else {
          smallestCycle.emplace(index, cycleSize);
        }
      }
    }
  }

  return smallestCycle;
}

std::vector<AtomIndex> makeRingIndexSequence(
  std::vector<BondIndex> edgeDescriptors
) {

  auto firstEdgeIter = edgeDescriptors.begin();
  // Initialize with first edge descriptor's indices
  std::vector<AtomIndex> indexSequence {
    firstEdgeIter->first,
    firstEdgeIter->second
  };

  indexSequence.reserve(edgeDescriptors.size());

  edgeDescriptors.erase(firstEdgeIter);
  // firstEdgeIter is now invalid!

  while(!edgeDescriptors.empty()) {
    for(
      auto edgeIter = edgeDescriptors.begin();
      edgeIter != edgeDescriptors.end();
      ++edgeIter
    ) {
      if(edgeIter->first == indexSequence.back()) {
        indexSequence.emplace_back(
          edgeIter->second
        );
        edgeDescriptors.erase(edgeIter);
        break;
      }

      if(edgeIter->second == indexSequence.back()) {
        indexSequence.emplace_back(
          edgeIter->first
        );
        edgeDescriptors.erase(edgeIter);
        break;
      }
    }
  }
  /* Now indexSequence should contain the entire sequence, but the first
   * vertex index occurs twice, once at the front and once at the back.
   */

  return indexSequence;
}

std::vector<AtomIndex> centralizeRingIndexSequence(
  std::vector<AtomIndex> ringIndexSequence,
  const AtomIndex center
) {
  assert(ringIndexSequence.front() == ringIndexSequence.back());

  ringIndexSequence.erase(ringIndexSequence.end() - 1);

  auto findIter = std::find(
    ringIndexSequence.begin(),
    ringIndexSequence.end(),
    center
  );

  assert(findIter != ringIndexSequence.end());

  std::rotate(
    ringIndexSequence.begin(),
    findIter,
    ringIndexSequence.end()
  );

  ringIndexSequence.push_back(
    center
  );

  return ringIndexSequence;
}


unsigned countPlanarityEnforcingBonds(
  const std::vector<BondIndex>& edgeSet,
  const OuterGraph& graph
) {
  return std::accumulate(
    std::begin(edgeSet),
    std::end(edgeSet),
    0u,
    [&graph](const unsigned carry, const BondIndex& edge) {
      if(graph.bondType(edge) == BondType::Double) {
        return carry + 1;
      }

      return carry;
    }
  );
}

} // namespace molassembler

} // namespace Scine
