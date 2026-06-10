#pragma once

#include <vector>

namespace tt {

// Kelly fraction for a binary bet: win prob p, net odds b (win b per 1 staked).
// f* = (b*p - (1-p)) / b.
double kelly_fraction(double p, double b);

// Continuous/Gaussian Kelly for an edge with mean mu and variance var: f* = mu/var.
double kelly_gaussian(double mu, double var);

// Fractional Kelly with a hard position cap: clamp(lambda * f_star, +/- cap).
double fractional_kelly(double f_star, double lambda, double cap);

// Volatility-target weight: sigma_target / sigma_hat, capped. (Sign applied by caller.)
double vol_target_weight(double sigma_target, double sigma_hat, double cap);

// Portfolio volatility sqrt(w' Sigma w); cov is row-major n*n.
double portfolio_volatility(const std::vector<double>& w, const std::vector<double>& cov);

// Equity from per-trade returns scaled by per-trade sizes. If max_dd_stop > 0,
// once drawdown from the running peak exceeds it, stop opening new positions
// (equity goes flat) — a simple risk kill-switch.
std::vector<double> sized_equity(const std::vector<double>& returns,
                                 const std::vector<double>& sizes,
                                 double max_dd_stop = 0.0);

}  // namespace tt
