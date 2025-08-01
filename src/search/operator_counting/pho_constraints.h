#ifndef OPERATOR_COUNTING_PHO_CONSTRAINTS_H
#define OPERATOR_COUNTING_PHO_CONSTRAINTS_H

#include "constraint_generator.h"

#include "../algorithms/named_vector.h"
#include "../pdbs/types.h"

#include <memory>

namespace plugins {
class Options;
}

namespace pdbs {
class PatternCollectionGenerator;
}

namespace operator_counting {
class PhOConstraints : public ConstraintGenerator {
    std::shared_ptr<pdbs::PatternCollectionGenerator> pattern_generator;

    int constraint_offset;
    std::shared_ptr<pdbs::PDBCollection> pdbs;
public:
    explicit PhOConstraints(
        const std::shared_ptr<pdbs::PatternCollectionGenerator> &patterns);

    virtual void initialize_constraints(
        const std::shared_ptr<AbstractTask> &task,
        lp::LinearProgram &lp) override;
    virtual bool update_constraints(
        const State &state, lp::LPSolver &lp_solver) override;
};
}

#endif
