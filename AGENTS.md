# Repository instructions

## Purpose and layout
This C++17 project prices European call and put options using Black-Scholes and
a Cox-Ross-Rubinstein (CRR) recombining binomial tree. `include/option/` holds
the public API, `src/` the library and CLI, `tests/` unit tests and independent
numerical references, `benchmarks/` timing experiments, `docs/` the model and
reproduction notes, and `scripts/` verification tools. Fixed upstream dependency
archives and licenses are in `third_party/`.

## Commands
- Configure: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`
- Build: `cmake --build build --config Release --parallel 2`
- Test: `ctest --test-dir build -C Release --output-on-failure`
- Benchmark: `cmake --build build --config Release --target run_benchmarks`
- Documentation: `cmake --build build --config Release --target docs`
- Full Linux/macOS check: `bash scripts/verify.sh`
- Example: `./build/bin/option_pricer --kind call --method black-scholes`

## Numerical and workflow rules
Use C++17 and Doxygen comments on public APIs. Rates are continuously compounded
annual decimals, maturity is in years, and volatility is annualized. Do not add
early exercise to the European tree. Keep price differences from approximation
separate from implementation errors: CRR is not exactly Black-Scholes at a finite
step count, and its error need not decrease monotonically at every step count.
Reject tree configurations with invalid risk-neutral probabilities. Build and
test changes; verify numerical changes against independent references. Report
only executed checks and measured timings. Preserve dependency licenses. Keep
changes within this repository; do not publish remotely or change permissions
without a request. Local results do not establish public CI or Pages success.

