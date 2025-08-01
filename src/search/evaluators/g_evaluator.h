#ifndef EVALUATORS_G_EVALUATOR_H
#define EVALUATORS_G_EVALUATOR_H

#include "../evaluator.h"

namespace g_evaluator {
class GEvaluator : public Evaluator {
public:
    GEvaluator(const std::string &description, utils::Verbosity verbosity);

    virtual EvaluationResult compute_result(
        EvaluationContext &eval_context) override;

    virtual void get_path_dependent_evaluators(
        std::set<Evaluator *> &) override {
    }
};
}

#endif
