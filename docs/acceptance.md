# Six-item acceptance checklist

Local execution and public deployment are recorded separately. The local record
is `verification/summary.json` with `verification/local-verify.log`; the public
record is the workflow run on the default branch. Both describe executed checks
only.

| Criterion | Points | Implementation | Public evidence |
|---|---:|---|---|
| Public repository, clear structure and README | 2 | Separate code, tests and benchmarks, with build/run instructions | [Public repository](https://github.com/lzm-opt/european-option-pricing-cpp) |
| CMake and automatic CI build | 4 | Library/CLI targets and the Linux/Windows workflow | Build jobs in [Actions](https://github.com/lzm-opt/european-option-pricing-cpp/actions) |
| C++ framework tests running in CI | 4 | doctest plus independent CLI and convergence checks | CTest and doctest steps of the same run |
| Automatic benchmark, results in CI log | 4 | Google Benchmark, paired workloads, JSON validator | Printed `run_benchmarks` output in the job log |
| Doxygen publication and README link | 4 | Checked HTML, Pages workflow and the link block in README | [Published documentation](https://lzm-opt.github.io/european-option-pricing-cpp/) |
| Root agent configuration | 2 | `AGENTS.md`, plus `CLAUDE.md` importing it | Repository root |

The assignment gives full credit for criteria 2-5 only after successful CI/CD
execution, so the links above matter as much as the sources. A local build or an
unexecuted workflow would not establish them.

Reproduce with `bash scripts/verify.sh`, or `./scripts/verify.ps1` in Windows
PowerShell with the required C++ toolchain. The benchmark has 16 workloads,
three repetitions each, producing 48 raw measurements and 64 aggregate rows.
The separate 32-row convergence experiment records approximation error.
