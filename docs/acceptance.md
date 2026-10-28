# Six-item acceptance checklist

Local execution and public deployment are recorded separately. The delivered
`verification/summary.json` and log describe actual local checks. An eventual
owner must publish the repository and obtain successful public workflow runs.

| Criterion | Points | Implementation | Public step remaining |
|---|---:|---|---|
| Public repository, clear structure and README | 2 | Separate code, tests and benchmarks, with build/run instructions | Publish the source in the owner's account |
| CMake and automatic CI build | 4 | Library/CLI targets and the Linux/Windows workflow | Successful GitHub build jobs |
| C++ framework tests running in CI | 4 | doctest plus independent CLI and convergence checks | Successful GitHub test jobs |
| Automatic benchmark, results in CI log | 4 | Google Benchmark, paired workloads, JSON validator | Successful benchmark jobs and saved artifacts |
| Doxygen publication and README link | 4 | Checked HTML, Pages workflow and real-link configuration helper | Enable Pages, deploy, and verify the actual link |
| Root agent configuration | 2 | `AGENTS.md` documents purpose, layout, commands and constraints | Include it in the published repository |

The assignment gives full credit for criteria 2-5 only after successful CI/CD
execution. A local build or an unexecuted workflow does not establish that.
Follow [Publishing](publishing.md) and retain the final commit, successful Actions
run and actual documentation URL with the submission.

Reproduce with `bash scripts/verify.sh`, or `./scripts/verify.ps1` in Windows
PowerShell with the required C++ toolchain. The benchmark has 16 workloads,
three repetitions each, producing 48 raw measurements and 64 aggregate rows.
The separate 32-row convergence experiment records approximation error.

