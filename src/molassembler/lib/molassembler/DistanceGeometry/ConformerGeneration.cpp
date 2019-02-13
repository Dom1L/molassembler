/*!@file
 * @copyright ETH Zurich, Laboratory for Physical Chemistry, Reiher Group.
 *   See LICENSE.txt
 */

#include "molassembler/DistanceGeometry/ConformerGeneration.h"

#include <dlib/optimization.h>
#include <Eigen/Dense>
#include "Utils/Constants.h"
#include "Utils/Typenames.h"

#include "molassembler/DistanceGeometry/dlibAdaptors.h"
#include "molassembler/DistanceGeometry/dlibDebugAdaptors.h"
#include "molassembler/DistanceGeometry/MetricMatrix.h"
#include "molassembler/DistanceGeometry/SpatialModel.h"
#include "molassembler/DistanceGeometry/RefinementProblem.h"
#include "molassembler/DistanceGeometry/Error.h"
#include "molassembler/DistanceGeometry/ExplicitGraph.h"
#include "molassembler/Graph/GraphAlgorithms.h"
#include "Utils/QuaternionFit.h"

namespace Scine {

namespace molassembler {

namespace DistanceGeometry {

namespace detail {

AngstromWrapper convertToAngstromWrapper(
  const dlib::matrix<double, 0, 1>& vectorizedPositions
) {
  const Eigen::Index dimensionality = 4;
  assert(vectorizedPositions.size() % dimensionality == 0);

  const unsigned N = vectorizedPositions.size() / dimensionality;
  AngstromWrapper angstromWrapper {N};
  for(unsigned i = 0; i < N; i++) {
    angstromWrapper.positions.row(i) = Scine::Utils::Position {
      vectorizedPositions(dimensionality * i),
      vectorizedPositions(dimensionality * i + 1),
      vectorizedPositions(dimensionality * i + 2)
    };
  }

  return angstromWrapper;
}

AngstromWrapper convertToAngstromWrapper(const Eigen::MatrixXd& positionsMatrix) {
  assert(positionsMatrix.cols() == 3);
  const unsigned N = positionsMatrix.rows();
  AngstromWrapper angstromWrapper {N};
  for(unsigned i = 0; i < N; ++i) {
    angstromWrapper.positions.row(i) = Scine::Utils::Position {
      positionsMatrix.row(i).transpose()
    };
  }
  return angstromWrapper;
}

Eigen::MatrixXd fitAndSetFixedPositions(
  const dlib::matrix<double, 0, 1>& vectorizedPositions,
  const Configuration& configuration
) {
  /* Fixed positions postprocessing:
   * - Rotate and translate the generated coordinates towards the fixed
   *   positions indicated for each.
   * - Assuming the fit isn't absolutely exact, overwrite the existing
   *   positions with the fixed ones
   */
  // Convert the vectorized dlib positions into a positions matrix
  const unsigned dimensionality = 4;
  const unsigned N = vectorizedPositions.size() / dimensionality;
  Eigen::MatrixXd positionMatrix(N, 3);
  for(unsigned i = 0; i < N; ++i) {
    positionMatrix.row(i) = Eigen::RowVector3d(
      vectorizedPositions(dimensionality * i),
      vectorizedPositions(dimensionality * i + 1),
      vectorizedPositions(dimensionality * i + 2)
    );
  }

  /* Construct a reference matrix from the fixed positions (these are still in
   * bohr!) and a weights vector
   */
  Eigen::MatrixXd referenceMatrix = Eigen::MatrixXd::Zero(N, 3);
  Eigen::VectorXd weights = Eigen::VectorXd::Zero(N);
  for(const auto& indexPositionPair : configuration.fixedPositions) {
    referenceMatrix.row(indexPositionPair.first) = indexPositionPair.second.transpose();
    weights(indexPositionPair.first) = 1;
  }

  referenceMatrix *= Scine::Utils::Constants::angstrom_per_bohr;

  // Perform the QuaternionFit
  Scine::Utils::QuaternionFit fit(referenceMatrix, positionMatrix, weights);

  return fit.getFittedData();
}

Molecule narrow(Molecule molecule) {
  const auto& stereopermutatorList = molecule.stereopermutators();

  do {
    /* If we change any stereopermutator, we must re-check if there are still
     * unassigned stereopermutators since assigning a stereopermutator can
     * invalidate the entire stereopermutator list (because stereopermutators
     * may appear or disappear on assignment due to ranking)
     */
    std::vector<AtomIndex> unassignedAtomStereopermutators;

    for(const auto& atomStereopermutator : stereopermutatorList.atomStereopermutators()) {
      if(!atomStereopermutator.assigned()) {
        unassignedAtomStereopermutators.push_back(
          atomStereopermutator.centralIndex()
        );
      }
    }

    if(!unassignedAtomStereopermutators.empty()) {
      unsigned choice = temple::random::getSingle<unsigned>(
        0,
        unassignedAtomStereopermutators.size() - 1,
        randomnessEngine()
      );

      molecule.assignStereopermutatorRandomly(
        unassignedAtomStereopermutators.at(choice)
      );

      // Re-check the loop condition
      continue;
    }

    std::vector<BondIndex> unassignedBondStereopermutators;

    for(const auto& bondStereopermutator : stereopermutatorList.bondStereopermutators()) {
      if(!bondStereopermutator.assigned()) {
        unassignedBondStereopermutators.push_back(
          bondStereopermutator.edge()
        );
      }
    }

    if(!unassignedBondStereopermutators.empty()) {
      unsigned choice = temple::random::getSingle<unsigned>(
        0,
        unassignedBondStereopermutators.size() - 1,
        randomnessEngine()
      );

      molecule.assignStereopermutatorRandomly(
        unassignedBondStereopermutators.at(choice)
      );
    }

  } while(stereopermutatorList.hasUnassignedStereopermutators());

  return molecule;
}

} // namespace detail

MoleculeDGInformation gatherDGInformation(
  const Molecule& molecule,
  const Configuration& configuration
) {
  // Generate a spatial model from the molecular graph and stereopermutators
  SpatialModel spatialModel {molecule, configuration};

  // Extract gathered data
  MoleculeDGInformation data;
  data.bounds = spatialModel.makeBoundsList();
  data.chiralityConstraints = spatialModel.getChiralityConstraints();
  data.dihedralConstraints = spatialModel.getDihedralConstraints();

  return data;
}

MoleculeDGInformation gatherDGInformation(
  const Molecule& molecule,
  const Configuration& configuration,
  std::string& spatialModelGraphvizString
) {
  // Generate a spatial model from the molecular graph and stereopermutators
  SpatialModel spatialModel {molecule, configuration};
  spatialModelGraphvizString = spatialModel.dumpGraphviz();

  // Extract gathered data
  MoleculeDGInformation data;
  data.bounds = spatialModel.makeBoundsList();
  data.chiralityConstraints = spatialModel.getChiralityConstraints();
  data.dihedralConstraints = spatialModel.getDihedralConstraints();

  return data;
}

// Debug version
std::list<RefinementData> debugRefinement(
  const Molecule& molecule,
  const unsigned numConformers,
  const Configuration& configuration
) {
  if(molecule.stereopermutators().hasZeroAssignmentStereopermutators()) {
    Log::log(Log::Level::Warning)
      << "This molecule has stereopermutators with zero valid permutations!"
      << std::endl;
  }

  SpatialModel::checkFixedPositionsPreconditions(molecule, configuration);

  std::list<RefinementData> refinementList;

  /* In case the molecule has unassigned stereopermutators that are not trivially
   * assignable (u/1 -> 0/1), random assignments have to be made prior to calling
   * gatherDGInformation (which creates the DistanceBoundsMatrix via the
   * SpatialModel, which expects all stereopermutators to be assigned).
   * Accordingly, gatherDGInformation has to be repeated in those cases, while
   * it is necessary only once in the other
   */

  /* There is also some degree of doubt about the relative frequencies of
   * assignments in stereopermutation. I should get on that too.
   */

  bool regenerateEachStep = molecule.stereopermutators().hasUnassignedStereopermutators();

  MoleculeDGInformation DGData;
  std::string spatialModelGraphviz;

  if(!regenerateEachStep) { // Collect once, keep all the time
    DGData = gatherDGInformation(molecule, configuration, spatialModelGraphviz);
  }

  unsigned failures = 0;
  for(
    unsigned currentStructureNumber = 0;
    // Failed optimizations do not count towards successful completion
    currentStructureNumber < numConformers;
    ++currentStructureNumber
  ) {
    if(regenerateEachStep) {
      auto moleculeCopy = detail::narrow(molecule);

      if(moleculeCopy.stereopermutators().hasZeroAssignmentStereopermutators()) {
        Log::log(Log::Level::Warning)
          << "After setting stereopermutators at random, this molecule has "
          << "stereopermutators with zero valid permutations!"
          << std::endl;
      }

      // Fetch the DG data from the molecule with no unassigned stereopermutators
      DGData = gatherDGInformation(
        moleculeCopy,
        configuration,
        spatialModelGraphviz
      );
    }

    std::list<RefinementStepData> refinementSteps;

    ExplicitGraph explicitGraph {
      molecule,
      DGData.bounds
    };

    auto distanceBoundsResult = explicitGraph.makeDistanceBounds();
    if(!distanceBoundsResult) {
      Log::log(Log::Level::Warning) << "Failure in distance bounds matrix construction: "
        << distanceBoundsResult.error().message() << "\n";
      failures += 1;

      if(regenerateEachStep) {
        auto moleculeCopy = detail::narrow(molecule);

        SpatialModel model {moleculeCopy, configuration};
        model.writeGraphviz("DG-failure-spatial-model-" + std::to_string(currentStructureNumber) + ".dot");
      } else {
        SpatialModel model {molecule, configuration};
        model.writeGraphviz("DG-failure-spatial-model-" + std::to_string(currentStructureNumber) + ".dot");
      }

      continue;
    }

    DistanceBoundsMatrix distanceBounds {std::move(distanceBoundsResult.value())};

    /* No need to smooth the distance bounds, ExplicitGraph creates it
     * so that the triangle inequalities are fulfilled
     */

    assert(distanceBounds.boundInconsistencies() == 0);

    auto distanceMatrixResult = explicitGraph.makeDistanceMatrix(
      randomnessEngine(),
      configuration.partiality
    );
    if(!distanceMatrixResult) {
      Log::log(Log::Level::Warning) << "Failure in distance matrix construction.\n";
      failures += 1;
    }

    // Make a metric matrix from a generated distances matrix
    MetricMatrix metric(
      std::move(distanceMatrixResult.value())
    );

    // Get a position matrix by embedding the metric matrix
    auto embeddedPositions = metric.embed();

    // Vectorize and transfer to dlib
    errfValue<true>::Vector dlibPositions = dlib::mat(
      static_cast<Eigen::VectorXd>(
        Eigen::Map<Eigen::VectorXd>(
          embeddedPositions.data(),
          embeddedPositions.cols() * embeddedPositions.rows()
        )
      )
    );

    /* If a count of chirality constraints reveals that more than half are
     * incorrect, we can invert the structure (by multiplying e.g. all y
     * coordinates with -1) and then have more than half of chirality
     * constraints correct! In the count, chirality constraints with a target
     * value of zero are not considered (this would skew the count as those
     * chirality constraints should not have to pass an energetic maximum to
     * converge properly as opposed to tetrahedra with volume).
     */
    if(
      errfDetail::proportionChiralityConstraintsCorrectSign(
        DGData.chiralityConstraints,
        dlibPositions
      ) < 0.5
    ) {
      // Invert y coordinates
      for(unsigned i = 0; i < distanceBounds.N(); i++) {
        dlibPositions(
          static_cast<dlibIndexType>(i) * 4 + 1
        ) *= -1;
      }
    }

    dlib::matrix<double, 0, 0> squaredBounds = dlib::mat(
      static_cast<Eigen::MatrixXd>(
        distanceBounds.access().cwiseProduct(distanceBounds.access())
      )
    );

    /* Our embedded coordinates are four dimensional. Now we want to make sure
     * that all chiral constraints are correct, allowing the structure to expand
     * into the fourth spatial dimension if necessary to allow inversion.
     *
     * This stage of refinement is only needed if not all chirality constraints
     * are already correct (or there are none).
     */
    if(
      errfDetail::proportionChiralityConstraintsCorrectSign(
        DGData.chiralityConstraints,
        dlibPositions
      ) < 1
    ) {
      errfValue<false> valueFunctor {
        squaredBounds,
        DGData.chiralityConstraints,
        DGData.dihedralConstraints
      };

      dlibAdaptors::debugIterationOrAllChiralitiesCorrectStrategy inversionStopStrategy {
        configuration.refinementStepLimit,
        refinementSteps,
        valueFunctor
      };

      try {
        dlib::find_min(
          dlib::bfgs_search_strategy(),
          inversionStopStrategy,
          valueFunctor,
          errfGradient<false>(
            squaredBounds,
            DGData.chiralityConstraints,
            DGData.dihedralConstraints
          ),
          dlibPositions,
          0
        );
      } catch(std::out_of_range& e) {
        Log::log(Log::Level::Warning)
          << "Non-finite contributions to dihedral error function gradient.\n";
        failures += 1;
        continue;
      }

      // Handle inversion failure (hit step limit)
      if(
        inversionStopStrategy.iterations >= configuration.refinementStepLimit
        || errfDetail::proportionChiralityConstraintsCorrectSign(
          DGData.chiralityConstraints,
          dlibPositions
        ) < 1.0
      ) {
        Log::log(Log::Level::Warning)
          << "[" << currentStructureNumber << "]: "
          << "First stage of refinement fails. Loosening factor was "
          << configuration.spatialModelLoosening
          <<  "\n";
        failures += 1;
        continue; // this triggers a new structure to be generated
      }
    }

    /* Set up the second stage of refinement where we compress out the fourth
     * dimension that we allowed expansion into to invert the chiralities.
     */

    errfValue<true> refinementValueFunctor {
      squaredBounds,
      DGData.chiralityConstraints,
      DGData.dihedralConstraints
    };

    dlibAdaptors::debugIterationOrGradientNormStopStrategy refinementStopStrategy {
      configuration.refinementStepLimit,
      configuration.refinementGradientTarget,
      refinementSteps,
      refinementValueFunctor
    };

    try {
      dlib::find_min(
        dlib::bfgs_search_strategy(),
        refinementStopStrategy,
        refinementValueFunctor,
        errfGradient<true>(
          squaredBounds,
          DGData.chiralityConstraints,
          DGData.dihedralConstraints
        ),
        dlibPositions,
        0
      );
    } catch(std::out_of_range& e) {
      Log::log(Log::Level::Warning)
        << "Non-finite contributions to dihedral error function gradient.\n";
      failures += 1;
      continue;
    }

    bool reachedMaxIterations = refinementStopStrategy.iterations >= configuration.refinementStepLimit;
    bool notAllChiralitiesCorrect = errfDetail::proportionChiralityConstraintsCorrectSign(
      DGData.chiralityConstraints,
      dlibPositions
    ) < 1;
    bool structureAcceptable = errfDetail::finalStructureAcceptable(
      distanceBounds,
      DGData.chiralityConstraints,
      DGData.dihedralConstraints,
      dlibPositions
    );

    if(Log::particulars.count(Log::Particulars::DGFinalErrorContributions) > 0) {
      errfDetail::explainFinalContributions(
        distanceBounds,
        DGData.chiralityConstraints,
        DGData.dihedralConstraints,
        dlibPositions
      );
    }

    RefinementData refinementData;
    refinementData.steps = std::move(refinementSteps);
    refinementData.constraints = DGData.chiralityConstraints;
    refinementData.looseningFactor = configuration.spatialModelLoosening;
    refinementData.isFailure = (reachedMaxIterations || notAllChiralitiesCorrect || !structureAcceptable);
    refinementData.spatialModelGraphviz = spatialModelGraphviz;

    refinementList.push_back(
      std::move(refinementData)
    );

    if(reachedMaxIterations || notAllChiralitiesCorrect || !structureAcceptable) {
      Log::log(Log::Level::Warning)
        << "[" << currentStructureNumber << "]: "
        << "Second stage of refinement fails. Loosening factor was "
        << configuration.spatialModelLoosening
        <<  "\n";
      if(reachedMaxIterations) {
        Log::log(Log::Level::Warning) << "- Reached max iterations.\n";
      }

      if(notAllChiralitiesCorrect) {
        Log::log(Log::Level::Warning) << "- Not all chirality constraints have the correct sign.\n";
      }

      if(!structureAcceptable) {
        Log::log(Log::Level::Warning) << "- The final structure is unacceptable.\n";
        if(Log::isSet(Log::Particulars::DGStructureAcceptanceFailures)) {
          errfDetail::explainAcceptanceFailure(
            distanceBounds,
            DGData.chiralityConstraints,
            DGData.dihedralConstraints,
            dlibPositions
          );
        }
      }

      failures += 1;
    }
  }

  return refinementList;
}

outcome::result<AngstromWrapper> refine(
  Eigen::MatrixXd embeddedPositions,
  const DistanceBoundsMatrix& distanceBounds,
  const Configuration& configuration,
  const std::shared_ptr<MoleculeDGInformation>& DGDataPtr
) {
  // Vectorize and transfer to dlib
  errfValue<true>::Vector dlibPositions = dlib::mat(
    static_cast<Eigen::VectorXd>(
      Eigen::Map<Eigen::VectorXd>(
        embeddedPositions.data(),
        embeddedPositions.cols() * embeddedPositions.rows()
      )
    )
  );

  /* If a count of chirality constraints reveals that more than half are
   * incorrect, we can invert the structure (by multiplying e.g. all y
   * coordinates with -1) and then have more than half of chirality
   * constraints correct! In the count, chirality constraints with a target
   * value of zero are not considered (this would skew the count as those
   * chirality constraints should not have to pass an energetic maximum to
   * converge properly as opposed to tetrahedra with volume).
   */
  if(
    errfDetail::proportionChiralityConstraintsCorrectSign(
      DGDataPtr->chiralityConstraints,
      dlibPositions
    ) < 0.5
  ) {
    // Invert y coordinates
    for(unsigned i = 0; i < distanceBounds.N(); ++i) {
      dlibPositions(
        static_cast<dlibIndexType>(i) * 4  + 1
      ) *= -1;
    }
  }

  // Create the squared bounds matrix for use in refinement
  dlib::matrix<double, 0, 0> squaredBounds = dlib::mat(
    static_cast<Eigen::MatrixXd>(
      distanceBounds.access().cwiseProduct(distanceBounds.access())
    )
  );

  /* Refinement without penalty on fourth dimension only necessary if not all
   * chiral centers are correct. Of course, for molecules without chiral
   * centers at all, this stage is unnecessary
   */
  if(
    errfDetail::proportionChiralityConstraintsCorrectSign(
      DGDataPtr->chiralityConstraints,
      dlibPositions
    ) < 1
  ) {
    dlibAdaptors::iterationOrAllChiralitiesCorrectStrategy inversionStopStrategy {
      DGDataPtr->chiralityConstraints,
      configuration.refinementStepLimit
    };

    try {
      dlib::find_min(
        dlib::bfgs_search_strategy(),
        inversionStopStrategy,
        errfValue<false>(
          squaredBounds,
          DGDataPtr->chiralityConstraints,
          DGDataPtr->dihedralConstraints
        ),
        errfGradient<false>(
          squaredBounds,
          DGDataPtr->chiralityConstraints,
          DGDataPtr->dihedralConstraints
        ),
        dlibPositions,
        0
      );
    } catch(std::out_of_range& e) {
      return DGError::RefinementException;
    }

    if(inversionStopStrategy.iterations >= configuration.refinementStepLimit) {
      return DGError::RefinementMaxIterationsReached;
    }

    if(
      errfDetail::proportionChiralityConstraintsCorrectSign(
        DGDataPtr->chiralityConstraints,
        dlibPositions
      ) < 1.0
    ) {
      return DGError::RefinedChiralsWrong;
    }
  }

  dlibAdaptors::iterationOrGradientNormStopStrategy refinementStopStrategy {
    configuration.refinementStepLimit,
    configuration.refinementGradientTarget
  };

  // Refinement with penalty on fourth dimension is always necessary
  try {
    dlib::find_min(
      dlib::bfgs_search_strategy(),
      refinementStopStrategy,
      errfValue<true>(
        squaredBounds,
        DGDataPtr->chiralityConstraints,
        DGDataPtr->dihedralConstraints
      ),
      errfGradient<true>(
        squaredBounds,
        DGDataPtr->chiralityConstraints,
        DGDataPtr->dihedralConstraints
      ),
      dlibPositions,
      0
    );
  } catch(std::out_of_range& e) {
    return DGError::RefinementException;
  }

  /* Error conditions */
  // Max iterations reached
  if(refinementStopStrategy.iterations >= configuration.refinementStepLimit) {
    return DGError::RefinementMaxIterationsReached;
  }

  // Not all chirality constraints have the right sign
  if(
    errfDetail::proportionChiralityConstraintsCorrectSign(
      DGDataPtr->chiralityConstraints,
      dlibPositions
    ) < 1
  ) {
    return DGError::RefinedChiralsWrong;
  }

  // Structure inacceptable
  if(
    !errfDetail::finalStructureAcceptable(
      distanceBounds,
      DGDataPtr->chiralityConstraints,
      DGDataPtr->dihedralConstraints,
      dlibPositions
    )
  ) {
    return DGError::RefinedStructureInacceptable;
  }

  if(!configuration.fixedPositions.empty()) {
    auto positionMatrix = detail::fitAndSetFixedPositions(dlibPositions, configuration);

    return detail::convertToAngstromWrapper(positionMatrix);
  }

  return detail::convertToAngstromWrapper(dlibPositions);
}

outcome::result<AngstromWrapper> generateConformer(
  const Molecule& molecule,
  const Configuration& configuration,
  std::shared_ptr<MoleculeDGInformation>& DGDataPtr,
  bool regenerateDGDataEachStep,
  random::Engine& engine
) {
  if(regenerateDGDataEachStep) {
    auto moleculeCopy = detail::narrow(molecule);

    if(moleculeCopy.stereopermutators().hasZeroAssignmentStereopermutators()) {
      return DGError::ZeroAssignmentStereopermutators;
    }

    DGDataPtr = std::make_shared<MoleculeDGInformation>(
      gatherDGInformation(moleculeCopy, configuration)
    );
  }

  ExplicitGraph explicitGraph {
    molecule,
    DGDataPtr->bounds
  };

  // Get distance bounds matrix from the graph
  auto distanceBoundsResult = explicitGraph.makeDistanceBounds();
  if(!distanceBoundsResult) {
    return distanceBoundsResult.as_failure();
  }

  DistanceBoundsMatrix distanceBounds {std::move(distanceBoundsResult.value())};

  /* There should be no need to smooth the distance bounds, because the graph
   * type ought to create them within the triangle inequality bounds:
   */
  assert(distanceBounds.boundInconsistencies() == 0);

  // Generate a distances matrix from the graph
  auto distanceMatrixResult = explicitGraph.makeDistanceMatrix(
    engine,
    configuration.partiality
  );
  if(!distanceMatrixResult) {
    return distanceMatrixResult.as_failure();
  }

  // Make a metric matrix from the distances matrix
  MetricMatrix metric(
    std::move(distanceMatrixResult.value())
  );

  // Get a position matrix by embedding the metric matrix
  auto embeddedPositions = metric.embed();

  /* Refinement */
  return refine(
    std::move(embeddedPositions),
    distanceBounds,
    configuration,
    DGDataPtr
  );
}

std::vector<
  outcome::result<AngstromWrapper>
> run(
  const Molecule& molecule,
  const unsigned numConformers,
  const Configuration& configuration
) {
  using ReturnType = std::vector<
    outcome::result<AngstromWrapper>
  >;

  // In case there are zero assignment stereopermutators, we give up immediately
  if(molecule.stereopermutators().hasZeroAssignmentStereopermutators()) {
    return ReturnType(numConformers, DGError::ZeroAssignmentStereopermutators);
  }

#ifdef _OPENMP
  /* Ensure the molecule's mutable properties are already generated so none are
   * generated on threaded const-access.
   */
  molecule.graph().inner().populateProperties();
#endif

  /* In case the molecule has unassigned stereopermutators, we need to randomly
   * assign them for each conformer generated prior to generating the distance
   * bounds matrix. If not, then modelling data can be kept across all
   * conformer generation runs since no randomness has entered the equation.
   */
  auto DGDataPtr = std::make_shared<MoleculeDGInformation>();
  bool regenerateEachStep = molecule.stereopermutators().hasUnassignedStereopermutators();
  if(!regenerateEachStep) {
    *DGDataPtr = gatherDGInformation(molecule, configuration);
  }

  ReturnType results;
  results.reserve(numConformers);

#pragma omp parallel
  {
    /* We have to distribute pseudo-randomness into each thread consistently,
     * so we provide each thread its own Engine, seeded with subsequent values
     * from the global Engine:
     */
#ifdef _OPENMP
    const unsigned nThreads = omp_get_num_threads();
    std::vector<random::Engine> randomnessEngines(nThreads);
#pragma omp single
    {
      for(unsigned i = 0; i < nThreads; ++i) {
        randomnessEngines.at(i).seed(
          randomnessEngine()()
        );
      }
    }
#endif

    /* Each thread has its own DGDataPtr, for the following reason: If we do
     * not need to regenerate the SpatialModel data, then having all threads
     * share access the underlying data to generate conformers is fine. If,
     * otherwise, we do need to regenerate the SpatialModel data for each
     * conformer, then each thread will reset its pointer to its self-generated
     * SpatialModel data, creating thread-private state.
     */
#pragma omp for firstprivate(DGDataPtr)
    for(unsigned i = 0; i < numConformers; ++i) {
      // Get thread-specific randomness engine reference
#ifdef _OPENMP
      random::Engine& engine = randomnessEngines.at(
        omp_get_thread_num()
      );
#else
      random::Engine& engine = randomnessEngine();
#endif

      /* We have to handle any and all exceptions here if this is a parallel
       * environment
       */
      try {
        // Generate the conformer
        auto conformerResult = generateConformer(
          molecule,
          configuration,
          DGDataPtr,
          regenerateEachStep,
          engine
        );

#pragma omp critical(collectConformer)
        {
          results.push_back(std::move(conformerResult));
        }
      } catch(...) {
#pragma omp critical(collectConformer)
        {
          // Add an unknown error
          results.push_back(static_cast<DGError>(0));
        }
      } // end catch
    } // end pragma omp for private(DGDataPtr)
  } // end pragma omp parallel

  return results;
}

} // namespace DistanceGeometry

} // namespace molassembler

} // namespace Scine
