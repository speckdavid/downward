#ifndef LANDMARKS_LANDMARK_HEURISTIC_H
#define LANDMARKS_LANDMARK_HEURISTIC_H

#include "../heuristic.h"

#include "../tasks/default_value_axioms_task.h"
#include "../utils/hash.h"

class ConstBitsetView;

namespace successor_generator {
class SuccessorGenerator;
}

namespace landmarks {
class LandmarkFactory;
class LandmarkGraph;
class LandmarkNode;
class LandmarkStatusManager;

class LandmarkHeuristic : public Heuristic {
    bool initial_landmark_graph_has_cycle_of_natural_orderings;

protected:
    std::shared_ptr<LandmarkGraph> landmark_graph;
    const bool use_preferred_operators;
    // This map remains empty unless `use_preferred_operators` is true.
    utils::HashMap<FactPair, std::unordered_set<int>>
        landmarks_achieved_by_atom;

    std::unique_ptr<LandmarkStatusManager> landmark_status_manager;
    std::unique_ptr<successor_generator::SuccessorGenerator>
        successor_generator;

    void initialize(
        const std::shared_ptr<LandmarkFactory> &landmark_factory,
        bool prog_goal, bool prog_gn, bool prog_r);
    void compute_landmark_graph(
        const std::shared_ptr<LandmarkFactory> &landmark_factory);

    virtual int get_heuristic_value(const State &ancestor_state) = 0;

    bool operator_is_preferred(
        const OperatorProxy &op, const State &state, ConstBitsetView &future);
    void compute_landmarks_achieved_by_atom();
    void generate_preferred_operators(
        const State &state, ConstBitsetView &future);
    virtual int compute_heuristic(const State &ancestor_state) override;
public:
    LandmarkHeuristic(
        bool use_preferred_operators,
        const std::shared_ptr<AbstractTask> &transform, bool cache_estimates,
        const std::string &description, utils::Verbosity verbosity);

    virtual void get_path_dependent_evaluators(
        std::set<Evaluator *> &evals) override {
        evals.insert(this);
    }

    virtual void notify_initial_state(const State &initial_state) override;
    virtual void notify_state_transition(
        const State &parent_state, OperatorID op_id,
        const State &state) override;
};

extern void add_landmark_heuristic_options_to_feature(
    plugins::Feature &feature, const std::string &description);
extern std::tuple<
    std::shared_ptr<LandmarkFactory>, bool, bool, bool, bool,
    std::shared_ptr<AbstractTask>, bool, std::string, utils::Verbosity>
get_landmark_heuristic_arguments_from_options(const plugins::Options &opts);
}

#endif
