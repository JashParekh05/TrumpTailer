#pragma once

namespace tt {

inline constexpr const char* kVersion = "0.1.0";

// Build smoke test: proves the C++ core links and the pybind11 bridge works.
int add(int a, int b);

}  // namespace tt
