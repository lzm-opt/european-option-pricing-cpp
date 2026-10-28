# Runtime and approximation experiment

Measured locally on 26 October 2026 with Apple Clang 21.0.0, CMake Release
and Google Benchmark 1.9.5. Raw context and repetitions are preserved in
`verification/benchmark-report.json`. These are local measurements, not GitHub
CI results. Four synthetic contracts are specified in `data/scenarios.json`.

## Timing at a fixed tree resolution

Both methods price the same contract and option kind. CRR uses 512 steps. Values
below are median CPU time over three repetitions; setup and output are outside
timing, while validation and tree working-array allocation are included.

| Scenario | Kind | Analytic (us) | CRR 512 (us) | CRR / analytic |
|---|---|---:|---:|---:|
| at_the_money | call | 0.0173 | 17.147 | 992.4x |
| at_the_money | put | 0.0181 | 17.114 | 943.3x |
| dividend | call | 0.0193 | 17.089 | 887.4x |
| dividend | put | 0.0187 | 17.651 | 945.4x |
| out_of_the_money_call | call | 0.0189 | 17.188 | 910.7x |
| out_of_the_money_call | put | 0.0203 | 17.175 | 844.2x |
| negative_rate | call | 0.0185 | 17.199 | 930.5x |
| negative_rate | put | 0.0180 | 17.299 | 961.1x |

The formula has constant work; this backward tree has quadratic work in step
count and linear storage. The ratio is specific to the measured machine and
resolution. The methods do not have identical finite-step accuracy, so these
numbers are not an equal-accuracy speedup claim.

## Grid approximation error

The reference uses 80-digit Decimal arithmetic with a normal-density series,
independent of the C++ erf/erfc implementation. The table gives absolute price
error in currency units. Full 32-row data are in
`verification/convergence-report.json`.

| Scenario | Kind | 32 steps | 128 steps | 512 steps | 1024 steps |
|---|---|---:|---:|---:|---:|
| at_the_money | call | 0.06223797 | 0.01560742 | 0.00390475 | 0.00195261 |
| at_the_money | put | 0.06223797 | 0.01560742 | 0.00390475 | 0.00195261 |
| dividend | call | 0.00901893 | 0.01035193 | 0.00185174 | 0.00127899 |
| dividend | put | 0.00901893 | 0.01035193 | 0.00185174 | 0.00127899 |
| out_of_the_money_call | call | 0.07498804 | 0.01889337 | 0.00505733 | 0.00306108 |
| out_of_the_money_call | put | 0.07498804 | 0.01889337 | 0.00505733 | 0.00306108 |
| negative_rate | call | 0.04301554 | 0.00454657 | 0.00077476 | 0.00019559 |
| negative_rate | put | 0.04301554 | 0.00454657 | 0.00077476 | 0.00019559 |

The largest 1024-step absolute error in these scenarios was 0.0030610790.
The check requires less than 0.03 on that final grid. It does not require each
intermediate error to decrease; the recorded sequence can oscillate. This is a
finite scenario check, not an error bound for all accepted contracts.

The performance experiment has 16 workloads and three repetitions: 48 raw
measurement rows plus 64 aggregate rows, for 112 JSON entries. A second run from
the final source-transfer check is retained separately as
`verification/benchmark-reproduction.json` and `verification/convergence-reproduction.json`.

Reproduce with `bash scripts/verify.sh`; the script runs CTest/convergence,
the benchmark and JSON checker, and Doxygen with file/anchor validation. New
output is written under the selected build directory.
