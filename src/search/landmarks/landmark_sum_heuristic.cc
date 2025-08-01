#include "landmark_sum_heuristic.h"

#include "landmark.h"
#include "landmark_factory.h"
#include "landmark_status_manager.h"
#include "util.h"

#include "../plugins/plugin.h"
#include "../task_utils/successor_generator.h"
#include "../task_utils/task_properties.h"
#include "../utils/markup.h"

#include <limits>

using namespace std;

namespace landmarks {
static bool are_dead_ends_reliable(
    const shared_ptr<LandmarkFactory> &lm_factory,
    const TaskProxy &task_proxy) {
    if (task_properties::has_axioms(task_proxy)) {
        return false;
    }

    if (!lm_factory->supports_conditional_effects() &&
        task_properties::has_conditional_effects(task_proxy)) {
        return false;
    }

    return true;
}

LandmarkSumHeuristic::LandmarkSumHeuristic(
    const shared_ptr<LandmarkFactory> &lm_factory, bool pref, bool prog_goal,
    bool prog_gn, bool prog_r, const shared_ptr<AbstractTask> &transform,
    bool cache_estimates, const string &description, utils::Verbosity verbosity,
    tasks::AxiomHandlingType axioms)
    : LandmarkHeuristic(
          pref,
          tasks::get_default_value_axioms_task_if_needed(transform, axioms),
          cache_estimates, description, verbosity),
      dead_ends_reliable(are_dead_ends_reliable(lm_factory, task_proxy)) {
    if (log.is_at_least_normal()) {
        log << "Initializing landmark sum heuristic..." << endl;
    }
    initialize(lm_factory, prog_goal, prog_gn, prog_r);
    compute_landmark_costs();
}

int LandmarkSumHeuristic::get_min_cost_of_achievers(
    const unordered_set<int> &achievers) const {
    int min_cost = numeric_limits<int>::max();
    for (const int id : achievers) {
        OperatorProxy op = get_operator_or_axiom(task_proxy, id);
        min_cost = min(min_cost, op.get_cost());
    }
    return min_cost;
}

void LandmarkSumHeuristic::compute_landmark_costs() {
    /*
      For derived landmarks, we overapproximate that all operators are
      achievers. Since we do not want to explicitly store all operators
      in the achiever vector, we instead just compute the minimum cost
      over all operators and use this cost for all derived landmarks.
    */
    int min_operator_cost = task_properties::get_min_operator_cost(task_proxy);
    min_first_achiever_costs.reserve(landmark_graph->get_num_landmarks());
    min_possible_achiever_costs.reserve(landmark_graph->get_num_landmarks());
    for (const auto &node : *landmark_graph) {
        if (node->get_landmark().is_derived) {
            min_first_achiever_costs.push_back(min_operator_cost);
            min_possible_achiever_costs.push_back(min_operator_cost);
        } else {
            int min_first_achiever_cost =
                get_min_cost_of_achievers(node->get_landmark().first_achievers);
            min_first_achiever_costs.push_back(min_first_achiever_cost);
            int min_possible_achiever_cost = get_min_cost_of_achievers(
                node->get_landmark().possible_achievers);
            min_possible_achiever_costs.push_back(min_possible_achiever_cost);
        }
    }
}

int LandmarkSumHeuristic::get_heuristic_value(const State &ancestor_state) {
    int h = 0;
    ConstBitsetView past =
        landmark_status_manager->get_past_landmarks(ancestor_state);
    ConstBitsetView future =
        landmark_status_manager->get_future_landmarks(ancestor_state);
    for (int id = 0; id < landmark_graph->get_num_landmarks(); ++id) {
        if (future.test(id)) {
            const int min_achiever_cost = past.test(id)
                                              ? min_possible_achiever_costs[id]
                                              : min_first_achiever_costs[id];
            if (min_achiever_cost < numeric_limits<int>::max()) {
                h += min_achiever_cost;
            } else {
                return DEAD_END;
            }
        }
    }
    return h;
}

bool LandmarkSumHeuristic::dead_ends_are_reliable() const {
    return dead_ends_reliable;
}

class LandmarkSumHeuristicFeature
    : public plugins::TypedFeature<Evaluator, LandmarkSumHeuristic> {
public:
    LandmarkSumHeuristicFeature() : TypedFeature("landmark_sum") {
        document_title("Landmark sum heuristic");
        document_synopsis(
            "Formerly known as the landmark heuristic or landmark count "
            "heuristic.\n"
            "See the papers" +
            utils::format_conference_reference(
                {"Silvia Richter", "Malte Helmert", "Matthias Westphal"},
                "Landmarks Revisited",
                "https://ai.dmi.unibas.ch/papers/richter-et-al-aaai2008.pdf",
                "Proceedings of the 23rd AAAI Conference on Artificial "
                "Intelligence (AAAI 2008)",
                "975-982", "AAAI Press", "2008") +
            "and" +
            utils::format_journal_reference(
                {"Silvia Richter", "Matthias Westphal"},
                "The LAMA Planner: Guiding Cost-Based Anytime Planning with Landmarks",
                "http://www.aaai.org/Papers/JAIR/Vol39/JAIR-3903.pdf",
                "Journal of Artificial Intelligence Research", "39", "127-177",
                "2010"));
        /*
          We usually have the options of base classes behind the options
          of specific implementations. In the case of landmark
          heuristics, we decided to have the common options at the front
          because it feels more natural to specify the landmark factory
          before the more specific arguments like the used LP solver in
          the case of an optimal cost partitioning heuristic.
        */
        add_landmark_heuristic_options_to_feature(
            *this, "landmark_sum_heuristic");
        tasks::add_axioms_option_to_feature(*this);

        document_note(
            "Note on performance for satisficing planning",
            "The cost of a landmark is based on the cost of the operators that "
            "achieve it. For satisficing search this can be counterproductive "
            "since it is often better to focus on distance from goal (i.e. "
            "length of the plan) rather than cost. In experiments we achieved "
            "the best performance using the option "
            "'transform=adapt_costs(one)' to enforce unit costs.");
        document_note(
            "Preferred operators",
            "Computing preferred operators is *only enabled* when setting "
            "pref=true because it has a nontrivial runtime cost. Using the "
            "heuristic for preferred operators without setting pref=true "
            "has no effect.\n"
            "Our implementation to compute preferred operators based on "
            "landmarks differs from the description in the literature (see "
            "reference above). The original implementation computes two kinds "
            "of preferred operators:\n\n"
            "+ If there is an applicable operator that reaches a landmark, all "
            "such operators are preferred.\n"
            "+ If no such operators exist, perform an FF-style relaxed "
            "exploration towards the nearest landmarks (according to the "
            "landmark orderings) and use the preferred operators of this "
            "exploration.\n\n\n"
            "Our implementation only considers preferred operators of the "
            "first type and does not include the second type. The rationale "
            "for this change is that it reduces code complexity and helps more "
            "cleanly separate landmark-based and FF-based computations in "
            "LAMA-like planner configurations. In our experiments, only "
            "considering preferred operators of the first type reduces "
            "performance when using the heuristic and its preferred operators "
            "in isolation but improves performance when using this heuristic "
            "in conjunction with the FF heuristic, as in LAMA-like planner "
            "configurations.");

        document_language_support("action costs", "supported");
        document_language_support(
            "conditional_effects",
            "supported if the LandmarkFactory supports them; otherwise "
            "ignored");
        document_language_support("axioms", "supported");

        document_property("admissible", "no");
        document_property("consistent", "no");
        document_property(
            "safe", "yes except on tasks with conditional effects when "
                    "using a LandmarkFactory not supporting them");
    }

    virtual shared_ptr<LandmarkSumHeuristic> create_component(
        const plugins::Options &opts) const override {
        return plugins::make_shared_from_arg_tuples<LandmarkSumHeuristic>(
            get_landmark_heuristic_arguments_from_options(opts),
            tasks::get_axioms_arguments_from_options(opts));
    }
};

static plugins::FeaturePlugin<LandmarkSumHeuristicFeature> _plugin;
}
