# Algorithms and numerical checks

## Black-Scholes evaluation

The formula performs a fixed number of arithmetic and normal-CDF operations.
It uses constant work and storage. The implementation evaluates the normal CDF
with `erf`/`erfc` and handles zero expiry and zero volatility separately, avoiding a
division by zero in those limits.

Put-call parity is useful numerically as well as financially: computing the
out-of-the-money side first can avoid subtracting large near-equal terms when
recovering the in-the-money side. A negligible far-tail price can still round
to zero in finite precision.

## CRR backward induction

Keep one array of `N+1` terminal payoffs. On each backward layer, update each
active entry using its two children; entries beyond that layer are no longer
needed. Update order must preserve the previous layer's up-child value.

```text
for j = 0..N:
    values[j] = payoff(S * u^(2*j-N))
for layer = N-1 down to 0:
    for j = 0..layer:
        values[j] = discount * ((1-p)*values[j] + p*values[j+1])
return values[0]
```

There are `N*(N+1)/2` backward updates, so time is O(N squared) and storage
O(N). Recombining nodes avoid the exponential number of full price paths.
The model does not use the larger of continuation and immediate exercise values;
that would change this European calculation.

## Accuracy and speed are different measurements

Finite CRR and analytic prices should be close on sufficiently fine grids, not
exactly equal. The payoff kink can make even/odd grids or particular sequences
oscillate. The convergence check records the actual error at 32, 128, 512 and
1024 steps. It checks a stated final-grid tolerance for the supplied scenarios
without asserting that every intermediate error decreases.

The timing benchmark pairs the same four contracts and both option kinds.
CRR always uses 512 steps there. It compares the cost of two methods at that
fixed resolution; it does not claim equal accuracy or label CRR an optimization
of the formula. Scenario generation and console output are outside timing.

## Independent checks

The C++ unit tests use known prices, parity, price bounds, monotonicity, hand
trees and invalid inputs. The Python numerical oracle uses Decimal arithmetic
and a normal-CDF series, independently of the C++ `erfc` path. Small-tree prices
are also checked by summing terminal binomial probabilities, independently of
the backward-induction implementation.

Questions worth being able to explain:

1. Why does `p` depend on `r-q`? Discounted gains include the continuous dividend
   stream; the risk-neutral ex-dividend drift is `r-q`.
2. Why does the tree have O(N squared) time? Layer sizes add to a triangular sum.
3. Why is memory linear? Only one layer's values are retained.
4. Why can a tree be rejected while the formula works? A coarse discrete grid
   can violate its own probability constraint even for valid model parameters.
5. Why are finite-step differences not necessarily bugs? They include grid
   approximation error, separately from arithmetic and implementation error.
6. Why compare a fast formula with a tree? The experiment makes the cost/error
   tradeoff visible for two ways of evaluating the same European model.

