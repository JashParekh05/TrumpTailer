#include "tt/sizing.hpp"

#include <algorithm>
#include <cmath>

namespace tt {

namespace {
double clamp_abs(double x, double cap) {
    if (cap <= 0) return x;
    return std::max(-cap, std::min(cap, x));
}
}  // namespace

double kelly_fraction(double p, double b) {
    if (b <= 0) return 0.0;
    return (b * p - (1.0 - p)) / b;
}

double kelly_gaussian(double mu, double var) {
    return var > 0 ? mu / var : 0.0;
}

double fractional_kelly(double f_star, double lambda, double cap) {
    return clamp_abs(lambda * f_star, cap);
}

double vol_target_weight(double sigma_target, double sigma_hat, double cap) {
    double w = sigma_hat > 0 ? sigma_target / sigma_hat : 0.0;
    return clamp_abs(w, cap);
}

double portfolio_volatility(const std::vector<double>& w, const std::vector<double>& cov) {
    std::size_t n = w.size();
    double acc = 0.0;
    for (std::size_t i = 0; i < n; ++i)
        for (std::size_t j = 0; j < n; ++j) acc += w[i] * w[j] * cov[i * n + j];
    return acc > 0 ? std::sqrt(acc) : 0.0;
}

std::vector<double> sized_equity(const std::vector<double>& returns,
                                 const std::vector<double>& sizes, double max_dd_stop) {
    std::vector<double> equity;
    equity.reserve(returns.size());
    double e = 1.0, peak = 1.0;
    bool stopped = false;
    for (std::size_t i = 0; i < returns.size(); ++i) {
        double r = stopped ? 0.0 : sizes[i] * returns[i];
        e *= (1.0 + r);
        equity.push_back(e);
        peak = std::max(peak, e);
        if (max_dd_stop > 0.0 && peak > 0.0 && (peak - e) / peak > max_dd_stop) stopped = true;
    }
    return equity;
}

}  // namespace tt
