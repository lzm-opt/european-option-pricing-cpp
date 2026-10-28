# Pinned upstream dependencies

CMake checks the SHA-256 hashes of the included upstream release archives.
Dependency downloads are not required for a normal build. `dependencies.json`
records versions, source URLs and hashes; the adjacent license files are copied
from the unmodified upstream archives.

- doctest 2.5.3: MIT, `doctest-LICENSE.txt`.
- Google Benchmark 1.9.5: Apache-2.0, `benchmark-LICENSE.txt`.

The generic CMake, CI, verification scripts, and evidence validators are adapted
from the existing course-project infrastructure. Option pricing, test oracles,
synthetic scenarios and measurements belong to this separate implementation.
