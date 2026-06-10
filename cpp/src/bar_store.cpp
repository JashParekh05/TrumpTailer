#include "tt/bar_store.hpp"

#include <algorithm>
#include <stdexcept>

namespace tt {

void BarStore::load_symbol(const std::string& symbol, std::vector<Bar> bars) {
    for (std::size_t i = 1; i < bars.size(); ++i) {
        if (bars[i].ts_utc_ns <= bars[i - 1].ts_utc_ns) {
            throw std::invalid_argument(
                "BarStore::load_symbol: timestamps must be strictly increasing (" +
                symbol + ")");
        }
    }
    data_[symbol] = std::move(bars);
}

std::ptrdiff_t BarStore::index_asof(const std::string& symbol, int64_t ts) const {
    auto it = data_.find(symbol);
    if (it == data_.end()) return -1;
    const auto& v = it->second;
    // First bar with ts > query, then step back one: the as-of bar (ts <= query).
    auto upper = std::upper_bound(
        v.begin(), v.end(), ts,
        [](int64_t t, const Bar& b) { return t < b.ts_utc_ns; });
    if (upper == v.begin()) return -1;
    return std::distance(v.begin(), upper) - 1;
}

std::optional<Bar> BarStore::bar_asof(const std::string& symbol, int64_t ts) const {
    std::ptrdiff_t idx = index_asof(symbol, ts);
    if (idx < 0) return std::nullopt;
    return data_.at(symbol)[idx];
}

const std::vector<Bar>& BarStore::bars(const std::string& symbol) const {
    auto it = data_.find(symbol);
    if (it == data_.end()) {
        throw std::out_of_range("BarStore: unknown symbol " + symbol);
    }
    return it->second;
}

bool BarStore::has_symbol(const std::string& symbol) const {
    return data_.find(symbol) != data_.end();
}

std::vector<std::string> BarStore::symbols() const {
    std::vector<std::string> out;
    out.reserve(data_.size());
    for (const auto& [k, _] : data_) out.push_back(k);
    return out;
}

}  // namespace tt
