#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <stdexcept>

#include "tt/backtester.hpp"
#include "tt/bar_store.hpp"
#include "tt/clock.hpp"
#include "tt/costs.hpp"
#include "tt/event.hpp"
#include "tt/event_study.hpp"
#include "tt/metrics.hpp"
#include "tt/sizing.hpp"
#include "tt/version.hpp"

namespace py = pybind11;
using namespace tt;

namespace {

// Bulk-load OHLCV from numpy arrays (Python owns Parquet/pandas; C++ gets
// plain contiguous doubles). All arrays must share the bars' length.
void load_symbol_arrays(BarStore& store, const std::string& symbol,
                        py::array_t<int64_t> ts, py::array_t<double> open,
                        py::array_t<double> high, py::array_t<double> low,
                        py::array_t<double> close, py::array_t<double> volume) {
    auto n = ts.size();
    if (open.size() != n || high.size() != n || low.size() != n ||
        close.size() != n || volume.size() != n) {
        throw std::invalid_argument("load_symbol: all arrays must be equal length");
    }
    auto t = ts.unchecked<1>();
    auto o = open.unchecked<1>();
    auto h = high.unchecked<1>();
    auto l = low.unchecked<1>();
    auto c = close.unchecked<1>();
    auto v = volume.unchecked<1>();
    std::vector<Bar> bars(static_cast<std::size_t>(n));
    for (py::ssize_t i = 0; i < n; ++i) {
        bars[i] = Bar{t(i), o(i), h(i), l(i), c(i), v(i)};
    }
    store.load_symbol(symbol, std::move(bars));
}

}  // namespace

PYBIND11_MODULE(_tt_core, m) {
    m.doc() = "TrumpTailer C++ core: event-driven backtester, event-study "
              "statistics, sizing, metrics.";
    m.attr("__version__") = kVersion;
    m.def("add", &add, "Build smoke test", py::arg("a"), py::arg("b"));

    py::enum_<EventCategory>(m, "EventCategory")
        .value("TariffTrade", EventCategory::TariffTrade)
        .value("FedRates", EventCategory::FedRates)
        .value("SingleName", EventCategory::SingleName)
        .value("Geopolitical", EventCategory::Geopolitical)
        .value("Other", EventCategory::Other);

    py::enum_<Direction>(m, "Direction")
        .value("Bearish", Direction::Bearish)
        .value("Neutral", Direction::Neutral)
        .value("Bullish", Direction::Bullish);

    py::enum_<SessionType>(m, "SessionType")
        .value("RegularHours", SessionType::RegularHours)
        .value("PreMarket", SessionType::PreMarket)
        .value("AfterHours", SessionType::AfterHours)
        .value("Closed", SessionType::Closed);

    py::class_<Event>(m, "Event")
        .def(py::init<>())
        .def_readwrite("ts_utc_ns", &Event::ts_utc_ns)
        .def_readwrite("category", &Event::category)
        .def_readwrite("direction", &Event::direction)
        .def_readwrite("magnitude", &Event::magnitude)
        .def_readwrite("novelty", &Event::novelty)
        .def_readwrite("confidence", &Event::confidence)
        .def_readwrite("instrument", &Event::instrument)
        .def_readwrite("source_id", &Event::source_id);

    py::class_<Bar>(m, "Bar")
        .def(py::init<>())
        .def_readwrite("ts_utc_ns", &Bar::ts_utc_ns)
        .def_readwrite("open", &Bar::open)
        .def_readwrite("high", &Bar::high)
        .def_readwrite("low", &Bar::low)
        .def_readwrite("close", &Bar::close)
        .def_readwrite("volume", &Bar::volume);

    py::class_<BarStore>(m, "BarStore")
        .def(py::init<>())
        .def("load_symbol", &load_symbol_arrays, py::arg("symbol"), py::arg("ts"),
             py::arg("open"), py::arg("high"), py::arg("low"), py::arg("close"),
             py::arg("volume"))
        .def("bar_asof", &BarStore::bar_asof, py::arg("symbol"), py::arg("ts"))
        .def("index_asof", &BarStore::index_asof, py::arg("symbol"), py::arg("ts"))
        .def("has_symbol", &BarStore::has_symbol, py::arg("symbol"))
        .def("symbols", &BarStore::symbols)
        .def(
            "close_series",
            [](const BarStore& s, const std::string& sym) {
                const auto& b = s.bars(sym);
                py::array_t<double> out(static_cast<py::ssize_t>(b.size()));
                auto r = out.mutable_unchecked<1>();
                for (std::size_t i = 0; i < b.size(); ++i) r(i) = b[i].close;
                return out;
            },
            py::arg("symbol"), "Close prices as a numpy array (for trailing-vol sizing).");

    py::class_<TradingCalendar>(m, "TradingCalendar")
        .def(py::init<>())
        .def("classify", &TradingCalendar::classify, py::arg("ts_utc_ns"))
        .def("actionable_time", &TradingCalendar::actionable_time, py::arg("ts_utc_ns"))
        .def("next_open_after", &TradingCalendar::next_open_after, py::arg("ts_utc_ns"))
        .def("session_close", &TradingCalendar::session_close, py::arg("ts_utc_ns"));

    py::class_<EventStudyConfig>(m, "EventStudyConfig")
        .def(py::init<>())
        .def(py::init([](int es, int ee, int cs, int ce) {
                 return EventStudyConfig{es, ee, cs, ce};
             }),
             py::arg("est_start") = -120, py::arg("est_end") = -20,
             py::arg("car_start") = 0, py::arg("car_end") = 5)
        .def_readwrite("est_start", &EventStudyConfig::est_start)
        .def_readwrite("est_end", &EventStudyConfig::est_end)
        .def_readwrite("car_start", &EventStudyConfig::car_start)
        .def_readwrite("car_end", &EventStudyConfig::car_end);

    py::class_<EventStudyResult>(m, "EventStudyResult")
        .def_readonly("offsets", &EventStudyResult::offsets)
        .def_readonly("aar", &EventStudyResult::aar)
        .def_readonly("caar", &EventStudyResult::caar)
        .def_readonly("t_aar", &EventStudyResult::t_aar)
        .def_readonly("car_per_event", &EventStudyResult::car_per_event)
        .def_readonly("caar_total", &EventStudyResult::caar_total)
        .def_readonly("t_caar", &EventStudyResult::t_caar)
        .def_readonly("n", &EventStudyResult::n);

    m.def("event_study", &EventStudy::run, py::arg("symbols"), py::arg("event_bar_idx"),
          py::arg("store"), py::arg("benchmark"), py::arg("config"),
          "Market-model event study: AR/CAR/CAAR with cross-sectional t-stats.");

    // --- P4: costs, backtester, metrics ---
    py::class_<LinearCostModel>(m, "LinearCostModel")
        .def(py::init([](double hs, double comm, double imp, double adv) {
                 return LinearCostModel{hs, comm, imp, adv};
             }),
             py::arg("half_spread_bps") = 1.0, py::arg("commission_bps") = 0.5,
             py::arg("impact_bps") = 0.0, py::arg("adv_notional") = 1e9)
        .def_readwrite("half_spread_bps", &LinearCostModel::half_spread_bps)
        .def_readwrite("commission_bps", &LinearCostModel::commission_bps)
        .def_readwrite("impact_bps", &LinearCostModel::impact_bps)
        .def_readwrite("adv_notional", &LinearCostModel::adv_notional)
        .def("cost_bps", &LinearCostModel::cost_bps, py::arg("notional"));

    py::class_<BacktestConfig>(m, "BacktestConfig")
        .def(py::init([](int hold, int lat, LinearCostModel c) {
                 return BacktestConfig{hold, lat, c};
             }),
             py::arg("holding_bars") = 3, py::arg("latency_bars") = 0,
             py::arg("costs") = LinearCostModel{})
        .def_readwrite("holding_bars", &BacktestConfig::holding_bars)
        .def_readwrite("latency_bars", &BacktestConfig::latency_bars)
        .def_readwrite("costs", &BacktestConfig::costs);

    py::class_<Trade>(m, "Trade")
        .def_readonly("symbol", &Trade::symbol)
        .def_readonly("entry_ts", &Trade::entry_ts)
        .def_readonly("exit_ts", &Trade::exit_ts)
        .def_readonly("entry_px", &Trade::entry_px)
        .def_readonly("exit_px", &Trade::exit_px)
        .def_readonly("direction", &Trade::direction)
        .def_readonly("gross_ret", &Trade::gross_ret)
        .def_readonly("net_ret", &Trade::net_ret)
        .def_readonly("cost", &Trade::cost);

    py::class_<BacktestResult>(m, "BacktestResult")
        .def_readonly("trades", &BacktestResult::trades)
        .def_readonly("equity", &BacktestResult::equity)
        .def_readonly("gross_returns", &BacktestResult::gross_returns)
        .def_readonly("net_returns", &BacktestResult::net_returns)
        .def_readonly("gross_sharpe", &BacktestResult::gross_sharpe)
        .def_readonly("net_sharpe", &BacktestResult::net_sharpe)
        .def_readonly("total_net_return", &BacktestResult::total_net_return)
        .def_readonly("max_drawdown", &BacktestResult::max_drawdown)
        .def_readonly("hit_rate", &BacktestResult::hit_rate)
        .def_readonly("profit_factor", &BacktestResult::profit_factor)
        .def_readonly("n", &BacktestResult::n);

    m.def("backtest", &EventDrivenBacktester::run, py::arg("symbols"),
          py::arg("event_bar_idx"), py::arg("scores"), py::arg("store"), py::arg("config"),
          "Event-driven per-trade backtest, net of round-trip costs.");

    m.def("sharpe", &sharpe, py::arg("returns"));
    m.def("annualized_sharpe", &annualized_sharpe, py::arg("returns"), py::arg("periods_per_year"));
    m.def("max_drawdown", &max_drawdown, py::arg("equity"));
    m.def("normal_cdf", &normal_cdf, py::arg("x"));
    m.def("normal_ppf", &normal_ppf, py::arg("p"));
    m.def("probabilistic_sharpe", &probabilistic_sharpe, py::arg("returns"),
          py::arg("sr_benchmark") = 0.0);
    m.def("expected_max_sharpe", &expected_max_sharpe, py::arg("n_trials"),
          py::arg("var_sr_trials"));
    m.def("deflated_sharpe", &deflated_sharpe, py::arg("returns"), py::arg("n_trials"),
          py::arg("var_sr_trials"));

    // --- P5: sizing & risk ---
    m.def("kelly_fraction", &kelly_fraction, py::arg("p"), py::arg("b"));
    m.def("kelly_gaussian", &kelly_gaussian, py::arg("mu"), py::arg("var"));
    m.def("fractional_kelly", &fractional_kelly, py::arg("f_star"), py::arg("lambda_"),
          py::arg("cap"));
    m.def("vol_target_weight", &vol_target_weight, py::arg("sigma_target"),
          py::arg("sigma_hat"), py::arg("cap"));
    m.def("portfolio_volatility", &portfolio_volatility, py::arg("weights"), py::arg("cov"));
    m.def("sized_equity", &sized_equity, py::arg("returns"), py::arg("sizes"),
          py::arg("max_dd_stop") = 0.0);
}
