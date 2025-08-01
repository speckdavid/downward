#include "validation.h"

#include "../task_proxy.h"

#include "../utils/logging.h"
#include "../utils/system.h"

#include <algorithm>
#include <iostream>

using namespace std;
using utils::ExitCode;

namespace pdbs {
void validate_and_normalize_pattern(
    const TaskProxy &task_proxy, Pattern &pattern, utils::LogProxy &log) {
    /*
      - Sort by variable number and remove duplicate variables.
      - Warn if duplicate variables exist.
      - Error if patterns contain out-of-range variable numbers.
    */
    sort(pattern.begin(), pattern.end());
    auto it = unique(pattern.begin(), pattern.end());
    if (it != pattern.end()) {
        pattern.erase(it, pattern.end());
        if (log.is_warning()) {
            log << "Warning: duplicate variables in pattern have been removed"
                << endl;
        }
    }
    if (!pattern.empty()) {
        if (pattern.front() < 0) {
            cerr << "Variable number too low in pattern" << endl;
            utils::exit_with(ExitCode::SEARCH_CRITICAL_ERROR);
        }
        int num_variables = task_proxy.get_variables().size();
        if (pattern.back() >= num_variables) {
            cerr << "Variable number too high in pattern" << endl;
            utils::exit_with(ExitCode::SEARCH_CRITICAL_ERROR);
        }
    }
}

void validate_and_normalize_patterns(
    const TaskProxy &task_proxy, PatternCollection &patterns,
    utils::LogProxy &log) {
    /*
      - Validate and normalize each pattern (see there).
      - Warn if duplicate patterns exist.
    */
    for (Pattern &pattern : patterns)
        validate_and_normalize_pattern(task_proxy, pattern, log);
    PatternCollection sorted_patterns(patterns);
    sort(sorted_patterns.begin(), sorted_patterns.end());
    auto it = unique(sorted_patterns.begin(), sorted_patterns.end());
    if (it != sorted_patterns.end()) {
        if (log.is_warning()) {
            log << "Warning: duplicate patterns have been detected" << endl;
        }
    }
}
}
