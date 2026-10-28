#!/usr/bin/env python3
"""Check CLI output against independent references and record CRR convergence."""
import argparse
import csv
from decimal import Decimal, localcontext
import io
import json
from pathlib import Path
import subprocess
import sys

sys.dont_write_bytecode = True
from reference import PRECISION, black_scholes, terminal_tree

FLAGS = {"spot": "--spot", "strike": "--strike", "rate": "--rate",
         "dividend_yield": "--dividend", "volatility": "--volatility", "maturity": "--maturity"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--app", required=True, type=Path)
    parser.add_argument("--data-dir", required=True, type=Path)
    parser.add_argument("--convergence-output", type=Path)
    args = parser.parse_args()
    app = str(args.app.resolve())
    scenarios = json.loads((args.data_dir / "scenarios.json").read_text())["scenarios"]
    assert len(scenarios) == 4 and [s["id"] for s in scenarios] == list(range(4))
    counts = dict(analytic_comparisons=0, terminal_tree_comparisons=0,
                  convergence_comparisons=0, degenerate_comparisons=0, invalid_inputs=0)

    def execute(arguments, success=True):
        result = subprocess.run([app, *arguments], capture_output=True, text=True, timeout=30)
        if success:
            if result.returncode != 0 or result.stderr.strip():
                raise AssertionError(f"valid CLI failed: {arguments}: {result.stderr}")
        elif result.returncode == 0 or not result.stderr.strip() or result.stdout.strip():
            raise AssertionError(f"invalid CLI accepted/misreported: {arguments}")
        return result.stdout

    def run(contract, kind, method, steps=None):
        flags = [part for name, flag in FLAGS.items() for part in (flag, str(contract[name]))]
        flags += ["--kind", kind, "--method", method]
        if steps is not None:
            flags += ["--steps", str(steps)]
        output = execute(flags)
        rows = list(csv.reader(io.StringIO(output)))
        if rows[0] != ["metric", "value"] or any(len(row) != 2 for row in rows[1:]):
            raise AssertionError("malformed price CSV")
        metrics = dict(rows[1:])
        if len(metrics) != len(rows) - 1 or set(metrics) != {"method", "kind", "price", "steps"}:
            raise AssertionError("unexpected or duplicate output fields")
        if metrics["method"] != method or metrics["kind"] != kind:
            raise AssertionError("method or kind label mismatch")
        expected_steps = steps if method == "binomial" else 0
        if int(metrics["steps"]) != expected_steps:
            raise AssertionError("wrong reported step count")
        price = Decimal(metrics["price"])
        if not price.is_finite() or price < 0:
            raise AssertionError("price must be nonnegative and finite")
        return price

    def compare(actual, expected, label):
        with localcontext() as context:
            context.prec = PRECISION
            tolerance = max(abs(expected), Decimal(1)) * Decimal("5e-11")
            if abs(actual - expected) > tolerance:
                raise AssertionError(f"{label}: {actual} versus independent {expected}")

    convergence = []
    for scenario in scenarios:
        for kind in ("call", "put"):
            reference = black_scholes(scenario, kind)
            compare(run(scenario, kind, "black-scholes"), reference, "Black-Scholes")
            counts["analytic_comparisons"] += 1
            for steps in (1, 2, 8, 32):
                expected = terminal_tree(scenario, kind, steps)
                compare(run(scenario, kind, "binomial", steps), expected, "terminal-tree expectation")
                counts["terminal_tree_comparisons"] += 1
            for steps in (32, 128, 512, 1024):
                actual = run(scenario, kind, "binomial", steps)
                error = abs(actual - reference)
                convergence.append(dict(scenario=scenario["id"], kind=kind, steps=steps,
                                        analytic_reference=str(reference), tree_price=float(actual),
                                        absolute_error=float(error)))
                counts["convergence_comparisons"] += 1
                if steps == 1024 and error >= Decimal("0.03"):
                    raise AssertionError(f"1024-step approximation outside declared tolerance: {error}")

    for changes in ({"maturity": 0}, {"volatility": 0}):
        contract = dict(scenarios[0], **changes)
        for kind in ("call", "put"):
            expected = black_scholes(contract, kind)
            for method in ("black-scholes", "binomial"):
                actual = run(contract, kind, method, 17 if method == "binomial" else None)
                compare(actual, expected, "zero-expiry/volatility limit")
                counts["degenerate_comparisons"] += 1

    invalid = [
        ["--unknown", "1"], ["--spot"], ["--spot", "100", "--spot", "101"],
        ["--spot", "nan"], ["--rate", "inf"], ["--spot", "100x"], ["--spot", "0x10"],
        ["--strike", "0"], ["--spot", "-1"], ["--spot", "1e10"],
        ["--rate", "0.21"], ["--dividend", "-0.01"], ["--dividend", "0.3"],
        ["--volatility", "-0.1"], ["--volatility", "1.1"], ["--volatility", "0.00001"],
        ["--maturity", "-1"], ["--maturity", "11"], ["--maturity", "0.00001"],
        ["--kind", "american"], ["--method", "unknown"], ["--steps", "512"],
        ["--method", "binomial", "--steps", "0"],
        ["--method", "binomial", "--steps", "-1"],
        ["--method", "binomial", "--steps", "2049"],
        ["--method", "binomial", "--steps", "1.5"],
        ["--method", "binomial", "--steps", "4x"],
        ["--method", "binomial", "--steps", "99999999999999999999999999"],
        ["--method", "binomial", "--steps", "1", "--rate", "0.2", "--volatility", "0.0001"],
        ["--help", "extra"], ["--spot", ""],
    ]
    for arguments in invalid:
        execute(arguments, success=False)
        counts["invalid_inputs"] += 1
    help_text = execute(["--help"])
    if "--steps" not in help_text or "European" not in help_text:
        raise AssertionError("incomplete help")
    default_rows = dict(list(csv.reader(io.StringIO(execute([]))))[1:])
    if default_rows["kind"] != "call" or default_rows["method"] != "black-scholes":
        raise AssertionError("wrong CLI defaults")
    compare(Decimal(default_rows["price"]), black_scholes(scenarios[0], "call"), "default contract")
    result = dict(reference_precision=PRECISION,
                  reference_method="Decimal normal-density series; direct terminal binomial expectation",
                  records=convergence,
                  final_step_max_error=max(row["absolute_error"] for row in convergence if row["steps"] == 1024),
                  checks=counts)
    if args.convergence_output:
        args.convergence_output.parent.mkdir(parents=True, exist_ok=True)
        args.convergence_output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print("CLI validation passed: " + ", ".join(f"{value} {key}" for key, value in counts.items()) + ", defaults and help.")
    print(f"Convergence validation passed: {len(convergence)} price/error records; "
          f"largest 1024-step error {result['final_step_max_error']:.9g}. No monotone-error assumption.")


if __name__ == "__main__":
    main()
