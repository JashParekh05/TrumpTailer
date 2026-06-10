#pragma once

#include <string>
#include <vector>

#include "tt/bar_store.hpp"

namespace tt {

// Offsets are bar indices relative to the event bar (tau = 0). The estimation
// window [est_start, est_end) must end strictly before the event window so the
// market-model parameters never see the event itself.
struct EventStudyConfig {
    int est_start = -120;
    int est_end = -20;
    int car_start = 0;
    int car_end = 5;
};

struct EventStudyResult {
    std::vector<int> offsets;          // car_start .. car_end
    std::vector<double> aar;           // average abnormal return per offset
    std::vector<double> caar;          // cumulative AAR
    std::vector<double> t_aar;         // cross-sectional t-stat per offset
    std::vector<double> car_per_event; // CAR over the event window, per event
    double caar_total = 0.0;           // mean CAR across events
    double t_caar = 0.0;               // t-stat of the mean CAR
    int n = 0;                         // events actually used (with enough data)
};

// Market-model event study: AR = r_i - (alpha + beta * r_market), estimated per
// event over the estimation window, accumulated over the event window, then
// averaged cross-sectionally with t-statistics. Report n alongside every t.
class EventStudy {
public:
    // For event k: instrument symbols[k], with tau=0 at instrument bar
    // event_bar_idx[k]. Market returns come from `benchmark`, matched by
    // timestamp. Events without enough history/future bars are skipped.
    static EventStudyResult run(const std::vector<std::string>& symbols,
                                const std::vector<long long>& event_bar_idx,
                                const BarStore& store, const std::string& benchmark,
                                const EventStudyConfig& cfg);
};

}  // namespace tt
