#!/usr/bin/env python3
"""Check the complete, repeated Google Benchmark European option pricing experiment."""
import argparse
import collections
import json
import math
from pathlib import Path

SCENARIOS = (0, 1, 2, 3)
KINDS = (0, 1)
METHODS = ("BM_BlackScholes", "BM_CRR")
AGGREGATES = {"mean", "median", "stddev", "cv"}


def finite_number(value, *, positive=False):
    return (isinstance(value, (int, float)) and not isinstance(value, bool)
            and math.isfinite(value) and (value > 0 if positive else value >= 0))


def validate(path):
    data = json.loads(Path(path).read_text(encoding="utf-8"))
    rows = data.get("benchmarks") if isinstance(data, dict) else None
    if not isinstance(rows, list):
        raise ValueError("missing benchmark records")
    expected = {f"{method}/scenario:{scenario}/kind:{kind}"
                for method in METHODS for scenario in SCENARIOS for kind in KINDS}
    seen = collections.defaultdict(list)
    aggregates = collections.defaultdict(list)
    for row in rows:
        if not isinstance(row, dict) or row.get("error_occurred"):
            raise ValueError("invalid or failed benchmark record")
        name = row.get("run_name")
        if name not in expected:
            raise ValueError(f"unexpected workload: {name!r}")
        if row.get("repetitions") != 3 or row.get("threads") != 1:
            raise ValueError(f"invalid repetition/thread count: {name}")
        if not finite_number(row.get("iterations"), positive=True):
            raise ValueError(f"missing iterations: {name}")
        for key in ("real_time", "cpu_time"):
            if not finite_number(row.get(key)):
                raise ValueError(f"nonfinite or negative {key}: {name}")
        if row.get("time_unit") not in {"ns", "us", "ms", "s"}:
            raise ValueError(f"invalid time unit: {name}")
        if row.get("run_type") == "iteration":
            index = row.get("repetition_index")
            if (isinstance(index, bool) or not isinstance(index, int)
                    or index not in {0, 1, 2}):
                raise ValueError(f"invalid repetition index: {name}")
            if not all(finite_number(row.get(k), positive=True)
                       for k in ("real_time", "cpu_time")):
                raise ValueError(f"zero measured time: {name}")
            seen[name].append(index)
        elif row.get("run_type") == "aggregate":
            aggregate = row.get("aggregate_name")
            if aggregate not in AGGREGATES:
                raise ValueError(f"unexpected aggregate: {aggregate!r}")
            aggregates[name].append(aggregate)
        else:
            raise ValueError(f"unexpected run type: {row.get('run_type')!r}")
    if set(seen) != expected or set(aggregates) != expected:
        raise ValueError("missing workloads")
    for name in expected:
        if sorted(seen[name]) != [0, 1, 2]:
            raise ValueError(f"missing or duplicate repetitions: {name}")
        if len(aggregates[name]) != 4 or set(aggregates[name]) != AGGREGATES:
            raise ValueError(f"missing or duplicate aggregates: {name}")
    if len(rows) != 112:
        raise ValueError(f"expected 112 JSON records, got {len(rows)}")
    return len(expected), len(rows)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("json_file", type=Path)
    arguments = parser.parse_args()
    try:
        workloads, count = validate(arguments.json_file)
    except (OSError, ValueError, TypeError, KeyError) as error:
        parser.exit(1, f"Benchmark validation failed: {error}\n")
    print(f"Benchmark validation passed: {workloads} workloads, "
          f"48 measured repetitions, {count} JSON records.")


if __name__ == "__main__":
    main()
