#include "tt/metrics.hpp"

#include <algorithm>
#include <cmath>

namespace tt {

namespace {
constexpr double kEuler = 0.5772156649015329;  // Euler-Mascheroni

// Population n-th standardized central moment.
double std_moment(const std::vector<double>& x, double m, double sd, int p) {
    if (sd <= 0 || x.empty()) return 0.0;
    double s = 0.0;
    for (double v : x) s += std::pow((v - m) / sd, p);
    return s / x.size();
}
}  // namespace

double mean(const std::vector<double>& x) {
    if (x.empty()) return 0.0;
    double s = 0.0;
    for (double v : x) s += v;
    return s / x.size();
}

double stddev(const std::vector<double>& x, double m, int ddof) {
    if (static_cast<int>(x.size()) <= ddof) return 0.0;
    double ss = 0.0;
    for (double v : x) ss += (v - m) * (v - m);
    return std::sqrt(ss / (static_cast<double>(x.size()) - ddof));
}

double sharpe(const std::vector<double>& r) {
    if (r.size() < 2) return 0.0;
    double m = mean(r);
    double s = stddev(r, m, 1);
    return s > 0 ? m / s : 0.0;
}

double annualized_sharpe(const std::vector<double>& r, double periods) {
    return sharpe(r) * std::sqrt(periods);
}

double max_drawdown(const std::vector<double>& equity) {
    double peak = -1e300, mdd = 0.0;
    for (double e : equity) {
        peak = std::max(peak, e);
        if (peak > 0) mdd = std::max(mdd, (peak - e) / peak);
    }
    return mdd;
}

double hit_rate(const std::vector<double>& r) {
    if (r.empty()) return 0.0;
    int w = 0;
    for (double v : r) if (v > 0) ++w;
    return static_cast<double>(w) / r.size();
}

double profit_factor(const std::vector<double>& r) {
    double gain = 0.0, loss = 0.0;
    for (double v : r) (v >= 0 ? gain : loss) += std::fabs(v);
    return loss > 0 ? gain / loss : (gain > 0 ? 1e300 : 0.0);
}

double normal_cdf(double x) { return 0.5 * std::erfc(-x * M_SQRT1_2); }

double normal_ppf(double p) {
    static const double a[] = {-3.969683028665376e+01, 2.209460984245205e+02,
                               -2.759285104469687e+02, 1.383577518672690e+02,
                               -3.066479806614716e+01, 2.506628277459239e+00};
    static const double b[] = {-5.447609879822406e+01, 1.615858368580409e+02,
                               -1.556989798598866e+02, 6.680131188771972e+01,
                               -1.328068155288572e+01};
    static const double c[] = {-7.784894002430293e-03, -3.223964580411365e-01,
                               -2.400758277161838e+00, -2.549732539343734e+00,
                               4.374664141464968e+00, 2.938163982698783e+00};
    static const double d[] = {7.784695709041462e-03, 3.224671290700398e-01,
                               2.445134137142996e+00, 3.754408661907416e+00};
    const double plow = 0.02425, phigh = 1.0 - plow;
    if (p <= 0.0) return -1e300;
    if (p >= 1.0) return 1e300;
    if (p < plow) {
        double q = std::sqrt(-2.0 * std::log(p));
        return (((((c[0] * q + c[1]) * q + c[2]) * q + c[3]) * q + c[4]) * q + c[5]) /
               ((((d[0] * q + d[1]) * q + d[2]) * q + d[3]) * q + 1.0);
    }
    if (p <= phigh) {
        double q = p - 0.5, r = q * q;
        return (((((a[0] * r + a[1]) * r + a[2]) * r + a[3]) * r + a[4]) * r + a[5]) * q /
               (((((b[0] * r + b[1]) * r + b[2]) * r + b[3]) * r + b[4]) * r + 1.0);
    }
    double q = std::sqrt(-2.0 * std::log(1.0 - p));
    return -(((((c[0] * q + c[1]) * q + c[2]) * q + c[3]) * q + c[4]) * q + c[5]) /
           ((((d[0] * q + d[1]) * q + d[2]) * q + d[3]) * q + 1.0);
}

double probabilistic_sharpe(const std::vector<double>& returns, double sr_benchmark) {
    int T = static_cast<int>(returns.size());
    if (T < 3) return 0.0;
    double m = mean(returns);
    double sd = stddev(returns, m, 1);
    if (sd <= 0) return 0.0;
    double sr = m / sd;
    double skew = std_moment(returns, m, sd, 3);
    double kurt = std_moment(returns, m, sd, 4);  // non-excess (normal = 3)
    double denom = std::sqrt(1.0 - skew * sr + ((kurt - 1.0) / 4.0) * sr * sr);
    if (!(denom > 0)) return 0.0;
    return normal_cdf((sr - sr_benchmark) * std::sqrt(static_cast<double>(T - 1)) / denom);
}

double expected_max_sharpe(int n_trials, double var_sr_trials) {
    if (n_trials <= 1 || var_sr_trials <= 0) return 0.0;
    double N = static_cast<double>(n_trials);
    double a = normal_ppf(1.0 - 1.0 / N);
    double b = normal_ppf(1.0 - 1.0 / (N * M_E));
    return std::sqrt(var_sr_trials) * ((1.0 - kEuler) * a + kEuler * b);
}

double deflated_sharpe(const std::vector<double>& returns, int n_trials,
                       double var_sr_trials) {
    double sr0 = expected_max_sharpe(n_trials, var_sr_trials);
    return probabilistic_sharpe(returns, sr0);
}

}  // namespace tt
