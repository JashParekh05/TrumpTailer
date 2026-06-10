#pragma once

#include <algorithm>
#include <cmath>

namespace tt {

// Round-trip transaction cost in basis points. Base = half-spread + commission;
// optional square-root market-impact term scaled by participation
// (order notional / average daily $ volume). Headline returns are net of this.
struct LinearCostModel {
    double half_spread_bps = 1.0;
    double commission_bps = 0.5;
    double impact_bps = 0.0;     // coefficient on sqrt(participation)
    double adv_notional = 1e9;   // average daily $ volume

    double cost_bps(double notional) const {
        double partic = adv_notional > 0.0 ? notional / adv_notional : 0.0;
        return half_spread_bps + commission_bps +
               impact_bps * std::sqrt(std::max(0.0, partic));
    }
};

}  // namespace tt
