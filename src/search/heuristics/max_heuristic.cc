#include "max_heuristic.h"

#include "../plugins/plugin.h"
#include "../utils/logging.h"

#include <cassert>
#include <vector>

using namespace std;

namespace max_heuristic {
/*
  TODO: At the time of this writing, this shares huge amounts of code
        with h^add, and the two should be refactored so that the
        common code is only included once, in so far as this is
        possible without sacrificing run-time. We may want to avoid
        virtual calls in the inner-most loops; maybe a templated
        strategy pattern is an option. Right now, the only differences
        to the h^add code are the use of max() instead of add() and
        the lack of preferred operator support (but we might actually
        reintroduce that if it doesn't hurt performance too much).
 */

// construction and destruction
HSPMaxHeuristic::HSPMaxHeuristic(
    tasks::AxiomHandlingType axioms, const shared_ptr<AbstractTask> &transform,
    bool cache_estimates, const string &description, utils::Verbosity verbosity)
    : RelaxationHeuristic(
          axioms, transform, cache_estimates, description, verbosity) {
    if (log.is_at_least_normal()) {
        log << "Initializing HSP max heuristic..." << endl;
    }
}

// heuristic computation
void HSPMaxHeuristic::setup_exploration_queue() {
    queue.clear();

    for (Proposition &prop : propositions)
        prop.cost = -1;

    // Deal with operators and axioms without preconditions.
    for (UnaryOperator &op : unary_operators) {
        op.unsatisfied_preconditions = op.num_preconditions;
        op.cost = op.base_cost; // will be increased by precondition costs

        if (op.unsatisfied_preconditions == 0)
            enqueue_if_necessary(op.effect, op.base_cost);
    }
}

void HSPMaxHeuristic::setup_exploration_queue_state(const State &state) {
    for (FactProxy fact : state) {
        PropID init_prop = get_prop_id(fact);
        enqueue_if_necessary(init_prop, 0);
    }
}

void HSPMaxHeuristic::relaxed_exploration() {
    int unsolved_goals = goal_propositions.size();
    while (!queue.empty()) {
        pair<int, PropID> top_pair = queue.pop();
        int distance = top_pair.first;
        PropID prop_id = top_pair.second;
        Proposition *prop = get_proposition(prop_id);
        int prop_cost = prop->cost;
        assert(prop_cost >= 0);
        assert(prop_cost <= distance);
        if (prop_cost < distance)
            continue;
        if (prop->is_goal && --unsolved_goals == 0)
            return;
        for (OpID op_id : precondition_of_pool.get_slice(
                 prop->precondition_of, prop->num_precondition_occurences)) {
            UnaryOperator *unary_op = get_operator(op_id);
            unary_op->cost =
                max(unary_op->cost, unary_op->base_cost + prop_cost);
            --unary_op->unsatisfied_preconditions;
            assert(unary_op->unsatisfied_preconditions >= 0);
            if (unary_op->unsatisfied_preconditions == 0)
                enqueue_if_necessary(unary_op->effect, unary_op->cost);
        }
    }
}

int HSPMaxHeuristic::compute_heuristic(const State &ancestor_state) {
    State state = convert_ancestor_state(ancestor_state);

    setup_exploration_queue();
    setup_exploration_queue_state(state);
    relaxed_exploration();

    int total_cost = 0;
    for (PropID goal_id : goal_propositions) {
        const Proposition *goal = get_proposition(goal_id);
        int goal_cost = goal->cost;
        if (goal_cost == -1)
            return DEAD_END;
        total_cost = max(total_cost, goal_cost);
    }
    return total_cost;
}

class HSPMaxHeuristicFeature
    : public plugins::TypedFeature<Evaluator, HSPMaxHeuristic> {
public:
    HSPMaxHeuristicFeature() : TypedFeature("hmax") {
        document_title("Max heuristic");

        relaxation_heuristic::add_relaxation_heuristic_options_to_feature(
            *this, "hmax");

        document_language_support("action costs", "supported");
        document_language_support("conditional effects", "supported");
        document_language_support("axioms", "supported");

        document_property("admissible", "yes for tasks without axioms");
        document_property("consistent", "yes for tasks without axioms");
        document_property("safe", "yes");
        document_property("preferred operators", "no");
    }

    virtual shared_ptr<HSPMaxHeuristic> create_component(
        const plugins::Options &opts) const override {
        return plugins::make_shared_from_arg_tuples<HSPMaxHeuristic>(
            relaxation_heuristic::
                get_relaxation_heuristic_arguments_from_options(opts));
    }
};

static plugins::FeaturePlugin<HSPMaxHeuristicFeature> _plugin;
}
