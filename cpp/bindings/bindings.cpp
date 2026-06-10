#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <stdexcept>

#include "tt/bar_store.hpp"
#include "tt/clock.hpp"
#include "tt/event.hpp"
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
        .def("symbols", &BarStore::symbols);

    py::class_<TradingCalendar>(m, "TradingCalendar")
        .def(py::init<>())
        .def("classify", &TradingCalendar::classify, py::arg("ts_utc_ns"))
        .def("actionable_time", &TradingCalendar::actionable_time, py::arg("ts_utc_ns"))
        .def("next_open_after", &TradingCalendar::next_open_after, py::arg("ts_utc_ns"))
        .def("session_close", &TradingCalendar::session_close, py::arg("ts_utc_ns"));
}
