#ifndef PDBS_PATTERN_COLLECTION_GENERATOR_SYSTEMATIC_H
#define PDBS_PATTERN_COLLECTION_GENERATOR_SYSTEMATIC_H

#include "pattern_generator.h"
#include "types.h"

#include "../utils/hash.h"

#include <cstdlib>
#include <memory>
#include <unordered_set>
#include <vector>

class TaskProxy;

namespace causal_graph {
class CausalGraph;
}

namespace pdbs {
class CanonicalPDBsHeuristic;

// Invariant: patterns are always sorted.
class PatternCollectionGeneratorSystematic : public PatternCollectionGenerator {
    using PatternSet = utils::HashSet<Pattern>;

    const size_t max_pattern_size;
    const bool only_interesting_patterns;
    std::shared_ptr<PatternCollection> patterns;
    PatternSet pattern_set; // Cleared after pattern computation.

    void enqueue_pattern_if_new(const Pattern &pattern);
    void compute_eff_pre_neighbors(
        const causal_graph::CausalGraph &cg, const Pattern &pattern,
        std::vector<int> &result) const;
    void compute_connection_points(
        const causal_graph::CausalGraph &cg, const Pattern &pattern,
        std::vector<int> &result) const;

    void build_sga_patterns(
        const TaskProxy &task_proxy, const causal_graph::CausalGraph &cg);
    void build_patterns(const TaskProxy &task_proxy);
    void build_patterns_naive(const TaskProxy &task_proxy);
    virtual std::string name() const override;
    virtual PatternCollectionInformation compute_patterns(
        const std::shared_ptr<AbstractTask> &task) override;
public:
    PatternCollectionGeneratorSystematic(
        int pattern_max_size, bool only_interesting_patterns,
        utils::Verbosity verbosity);
};
}

#endif
