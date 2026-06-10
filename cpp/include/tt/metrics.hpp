#pragma once

#include <vector>

namespace tt {

double mean(const std::vector<double>& x);
double stddev(const std::vector<double>& x, double m, int ddof = 1);

// Non-annualized Sharpe (mean / sample std). Multiply by sqrt(periods/yr) to annualize.
double sharpe(const std::vector<double>& returns);
double annualized_sharpe(const std::vector<double>& returns, double periods_per_year);

double max_drawdown(const std::vector<double>& equity);  // >= 0, fraction
double hit_rate(const std::vector<double>& returns);
double profit_factor(const std::vector<double>& returns);

double normal_cdf(double x);
double normal_ppf(double p);  // inverse CDF (Acklam), 0 < p < 1

// Probabilistic Sharpe Ratio: P(true SR > sr_benchmark), adjusting for the
// sample's skew/kurtosis (Bailey & Lopez de Prado).
double probabilistic_sharpe(const std::vector<double>& returns, double sr_benchmark = 0.0);

// Expected maximum Sharpe under the null across n_trials independent configs.
double expected_max_sharpe(int n_trials, double var_sr_trials);

// Deflated Sharpe: PSR against the expected-max-Sharpe benchmark. n_trials is
// the number of strategy configurations tried; var_sr_trials is the variance of
// their Sharpe estimates. A marginal/negative DSR is a publishable result.
double deflated_sharpe(const std::vector<double>& returns, int n_trials,
                       double var_sr_trials);

}  // namespace tt
