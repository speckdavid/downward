#ifndef OPERATOR_COUNTING_LM_CUT_CONSTRAINTS_H
#define OPERATOR_COUNTING_LM_CUT_CONSTRAINTS_H

#include "constraint_generator.h"

#include <memory>

namespace lm_cut_heuristic {
class LandmarkCutLandmarks;
}

namespace operator_counting {
class LMCutConstraints : public ConstraintGenerator {
    std::unique_ptr<lm_cut_heuristic::LandmarkCutLandmarks> landmark_generator;
public:
    virtual void initialize_constraints(
        const std::shared_ptr<AbstractTask> &task,
        lp::LinearProgram &lp) override;
    virtual bool update_constraints(
        const State &state, lp::LPSolver &lp_solver) override;
};
}

#endif
