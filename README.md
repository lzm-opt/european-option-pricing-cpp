# European Option Pricing in C++

A C++17 calculator for European call and put options with continuous dividends.
It implements the Black-Scholes formula and a Cox-Ross-Rubinstein (CRR) binomial
tree, then measures runtime and finite-tree approximation error separately.
CMake, doctest, Google Benchmark, Doxygen and GitHub Actions are included.

<!-- repository-links:start -->
**Public repository and documentation:** not configured. This is a local project
ready to transfer. Follow [Publishing](docs/publishing.md) to set the eventual
owner's real links and enable CI/Pages. No public deployment is claimed.
<!-- repository-links:end -->

## Build and run

Requirements: a C++17 compiler, CMake 3.24 or later, a build tool, Python 3.9 or
later, and Doxygen. Fixed doctest and Google Benchmark release archives and their
licenses are bundled; CMake checks their SHA-256 hashes without downloading them.

Linux/macOS:

```sh
bash scripts/verify.sh
./build/bin/option_pricer --kind call --method black-scholes
./build/bin/option_pricer --kind call --method binomial --steps 512
```

Windows, in PowerShell with the Visual Studio C++ build tools available:

```powershell
./scripts/verify.ps1
./build/bin/option_pricer.exe --kind call --method black-scholes
./build/bin/option_pricer.exe --kind call --method binomial --steps 512
```

The complete script configures a Release build, compiles, runs CTest and numerical
checks, measures both methods, validates JSON, generates Doxygen, and checks its
local links. Any failure stops the script. An optional build directory keeps
generated files outside the source folder.

The default synthetic contract has spot 100, strike 100, annual rate 5%, dividend
yield zero, annual volatility 20%, and one year until expiry. Analytic prices are:

| Option | Black-Scholes price |
|---|---:|
| Call | 10.450583572186 |
| Put | 5.573526022257 |

The 512-step tree gives nearby prices, not exact copies of these numbers. A
larger tree normally improves the approximation envelope, but individual error
steps can oscillate. Recorded convergence results accompany the runtime report.

For a dividend-paying example:

```sh
./build/bin/option_pricer --spot 100 --strike 95 --rate 0.03 --dividend 0.02 --volatility 0.25 --maturity 0.5 --kind put --method binomial --steps 512
```

All rates are annual decimals: `0.05` means 5%, and `0.2` volatility means 20%.
Rates and dividends use continuous compounding. Maturity is a number of years.
Output is a `metric,value` CSV with method, option kind, price and step count.
Analytic output has step count zero. `--steps` is accepted only with the binomial
method; `--help` alone lists the options. Errors go to standard error and return
a nonzero exit status.

## Individual checks

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
cmake --build build --config Release --target run_benchmarks
cmake --build build --config Release --target docs
```

Outputs are `build/bin/`, `build/benchmark.json`, `build/convergence.json`, and
`build/docs/html/index.html`. Open the HTML index to browse the generated API.
The delivered `verification/` directory records the actual local checks and
contains measurement data and a portable HTML archive. Local evidence does not
establish Windows or GitHub execution.

| Path | Purpose |
|---|---|
| `include/option/pricing.hpp` | Public API, units and bounds |
| `src/pricing.cpp` | Analytic and CRR prices |
| `src/main.cpp` | Validated CLI and CSV output |
| `tests/` | C++ tests and independent numerical references |
| `benchmarks/` | Paired analytic/512-step workloads |
| `data/scenarios.json` | Four synthetic contract scenarios |
| `docs/` | Model, algorithms, convergence, timing and publishing |
| `scripts/` | Full verification and evidence checking |
| `.github/workflows/` | Three-platform CI and Pages deployment |
| `third_party/` | Pinned upstream archives and licenses |

Read [the financial model](docs/model.md), [the algorithms](docs/algorithms.md),
[the experiment report](docs/experiment-results.md), and
[the six-item checklist](docs/acceptance.md). The implementation has a bounded
input domain and European exercise only; it does not infer market parameters
or use real option quotes.

