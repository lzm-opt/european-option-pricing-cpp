#include "option/pricing.hpp"

#include <benchmark/benchmark.h>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
// Same synthetic contracts, in the same order, as data/scenarios.json.
constexpr std::array<option::Contract, 4> scenarios{{
    {100.0, 100.0, 0.05, 0.0, 0.2, 1.0},
    {100.0, 95.0, 0.03, 0.02, 0.25, 0.5},
    {90.0, 110.0, 0.02, 0.01, 0.3, 2.0},
    {110.0, 100.0, -0.01, 0.01, 0.2, 0.75}
}};
constexpr std::size_t tree_steps = 512;

option::Kind kind_for(int kind) {
    return kind == 0 ? option::Kind::call : option::Kind::put;
}

void workloads(benchmark::internal::Benchmark* experiment) {
    for (int scenario = 0; scenario < 4; ++scenario) {
        for (int kind = 0; kind < 2; ++kind) {
            experiment->Args({scenario, kind});
        }
    }
    experiment->ArgNames({"scenario", "kind"});
}

void BM_BlackScholes(benchmark::State& state) {
    const auto& contract = scenarios[static_cast<std::size_t>(state.range(0))];
    const auto kind = kind_for(static_cast<int>(state.range(1)));
    for (auto iteration : state) {
        (void)iteration;
        benchmark::DoNotOptimize(option::black_scholes(contract, kind));
    }
}

void BM_CRR(benchmark::State& state) {
    const auto& contract = scenarios[static_cast<std::size_t>(state.range(0))];
    const auto kind = kind_for(static_cast<int>(state.range(1)));
    for (auto iteration : state) {
        (void)iteration;
        benchmark::DoNotOptimize(option::crr_binomial(contract, kind, tree_steps));
    }
}

BENCHMARK(BM_BlackScholes)->Apply(workloads);
BENCHMARK(BM_CRR)->Apply(workloads);
} // namespace

int main(int argc, char** argv) {
    try {
        // CRR is approximate. Check an explicit pricing error budget rather
        // than falsely requiring exact equality with the analytic method.
        double max_error = 0.0;
        for (const auto& contract : scenarios) {
            for (auto kind : {option::Kind::call, option::Kind::put}) {
                const double analytic = option::black_scholes(contract, kind);
                const double tree = option::crr_binomial(contract, kind, tree_steps);
                const double error = std::abs(analytic - tree);
                if (!std::isfinite(error) || error > 0.05) {
                    throw std::runtime_error("512-step CRR exceeds the benchmark error budget");
                }
                if (error > max_error) max_error = error;
            }
        }
        std::cout << "Benchmark setup: CRR steps=" << tree_steps
                  << ", maximum absolute analytic/tree difference=" << max_error << '\n';
        benchmark::Initialize(&argc, argv);
        if (benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
        benchmark::AddCustomContext("crr_steps", "512");
        benchmark::AddCustomContext("pricing_error_budget", "0.05 currency units");
        benchmark::RunSpecifiedBenchmarks();
        benchmark::Shutdown();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Benchmark failed: " << error.what() << '\n';
        return 1;
    }
}
