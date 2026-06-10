#include "tt/event_study.hpp"

#include <cmath>
#include <optional>

namespace tt {

namespace {

// Log return of a bar series at index i (requires i >= 1).
double log_ret(const std::vector<Bar>& b, std::ptrdiff_t i) {
    return std::log(b[i].close / b[i - 1].close);
}

// Sample standard deviation (ddof = 1).
double sample_std(const std::vector<double>& x, double mean) {
    if (x.size() < 2) return 0.0;
    double ss = 0.0;
    for (double v : x) ss += (v - mean) * (v - mean);
    return std::sqrt(ss / (static_cast<double>(x.size()) - 1.0));
}

// Abnormal return of `inst` at offset `off` from event bar `idx`, using the
// market-model (alpha,beta) and the benchmark return on the same date.
// Returns nullopt if any required bar is missing.
std::optional<double> abnormal_return(const std::vector<Bar>& inst,
                                      const BarStore& store,
                                      const std::string& benchmark,
                                      std::ptrdiff_t idx, int off, double alpha,
                                      double beta) {
    std::ptrdiff_t ii = idx + off;
    if (ii < 1 || ii >= static_cast<std::ptrdiff_t>(inst.size())) return std::nullopt;
    std::int64_t ts = inst[ii].ts_utc_ns;
    std::ptrdiff_t jj = store.index_asof(benchmark, ts);
    const auto& mkt = store.bars(benchmark);
    if (jj < 1 || mkt[jj].ts_utc_ns != ts) return std::nullopt;  // need exact date match
    double ri = log_ret(inst, ii);
    double rm = log_ret(mkt, jj);
    return ri - (alpha + beta * rm);
}

}  // namespace

EventStudyResult EventStudy::run(const std::vector<std::string>& symbols,
                                 const std::vector<long long>& event_bar_idx,
                                 const BarStore& store, const std::string& benchmark,
                                 const EventStudyConfig& cfg) {
    EventStudyResult res;
    for (int off = cfg.car_start; off <= cfg.car_end; ++off) res.offsets.push_back(off);
    const std::size_t W = res.offsets.size();

    // ar_by_offset[k] holds each used event's AR at offset position k.
    std::vector<std::vector<double>> ar_by_offset(W);

    for (std::size_t e = 0; e < symbols.size(); ++e) {
        if (!store.has_symbol(symbols[e])) continue;
        const auto& inst = store.bars(symbols[e]);
        std::ptrdiff_t idx = static_cast<std::ptrdiff_t>(event_bar_idx[e]);

        // --- Estimate market model over [est_start, est_end). ---
        std::vector<double> xs, ys;  // market, instrument returns
        for (int off = cfg.est_start; off < cfg.est_end; ++off) {
            std::ptrdiff_t ii = idx + off;
            if (ii < 1 || ii >= static_cast<std::ptrdiff_t>(inst.size())) continue;
            std::int64_t ts = inst[ii].ts_utc_ns;
            std::ptrdiff_t jj = store.index_asof(benchmark, ts);
            const auto& mkt = store.bars(benchmark);
            if (jj < 1 || mkt[jj].ts_utc_ns != ts) continue;
            xs.push_back(log_ret(mkt, jj));
            ys.push_back(log_ret(inst, ii));
        }
        if (xs.size() < 2) continue;  // not enough estimation data

        double mx = 0, my = 0;
        for (std::size_t i = 0; i < xs.size(); ++i) { mx += xs[i]; my += ys[i]; }
        mx /= xs.size(); my /= ys.size();
        double cov = 0, var = 0;
        for (std::size_t i = 0; i < xs.size(); ++i) {
            cov += (xs[i] - mx) * (ys[i] - my);
            var += (xs[i] - mx) * (xs[i] - mx);
        }
        double beta = var > 0 ? cov / var : 0.0;
        double alpha = my - beta * mx;

        // --- Abnormal returns over the event window. ---
        std::vector<double> ar(W);
        bool complete = true;
        for (std::size_t k = 0; k < W; ++k) {
            auto a = abnormal_return(inst, store, benchmark, idx, res.offsets[k], alpha, beta);
            if (!a) { complete = false; break; }
            ar[k] = *a;
        }
        if (!complete) continue;

        double car = 0.0;
        for (std::size_t k = 0; k < W; ++k) { ar_by_offset[k].push_back(ar[k]); car += ar[k]; }
        res.car_per_event.push_back(car);
    }

    res.n = static_cast<int>(res.car_per_event.size());
    res.aar.assign(W, 0.0);
    res.caar.assign(W, 0.0);
    res.t_aar.assign(W, 0.0);
    if (res.n == 0) return res;

    double cumulative = 0.0;
    for (std::size_t k = 0; k < W; ++k) {
        double m = 0.0;
        for (double v : ar_by_offset[k]) m += v;
        m /= res.n;
        res.aar[k] = m;
        cumulative += m;
        res.caar[k] = cumulative;
        double s = sample_std(ar_by_offset[k], m);
        res.t_aar[k] = s > 0 ? m / (s / std::sqrt(static_cast<double>(res.n))) : 0.0;
    }

    double mc = 0.0;
    for (double v : res.car_per_event) mc += v;
    mc /= res.n;
    res.caar_total = mc;
    double sc = sample_std(res.car_per_event, mc);
    res.t_caar = sc > 0 ? mc / (sc / std::sqrt(static_cast<double>(res.n))) : 0.0;
    return res;
}

}  // namespace tt
