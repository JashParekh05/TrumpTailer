#include <pybind11/pybind11.h>

#include "tt/version.hpp"

namespace py = pybind11;

PYBIND11_MODULE(_tt_core, m) {
    m.doc() = "TrumpTailer C++ core: event-driven backtester, event-study "
              "statistics, sizing, metrics.";
    m.attr("__version__") = tt::kVersion;
    m.def("add", &tt::add, "Build smoke test", py::arg("a"), py::arg("b"));
}
