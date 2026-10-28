# Financial model and units

## European exercise

A call pays `max(S_T-K,0)` at maturity, and a put pays `max(K-S_T,0)`. Exercise
occurs only at expiry. The model takes spot `S`, strike `K`, annual risk-free
rate `r`, continuous dividend yield `q`, volatility `sigma`, and expiry `T` in
years. Parameters are constant, rates are continuously compounded, and all
prices use the same currency unit.

The textbook model assumes frictionless trading and a lognormal underlying.
Martin Haugh's [Columbia lecture notes](https://www.columbia.edu/~mh2078/FoundationsFE/BlackScholes.pdf),
sections 1 and 1.2, give the standard formula and continuous-dividend extension.
This project evaluates that specified model; it does not calibrate it to prices.

## Analytic formula

For positive expiry and volatility, with `Phi` the standard normal CDF:

```text
d1 = (log(S/K) + (r-q+sigma*sigma/2)*T) / (sigma*sqrt(T))
d2 = d1 - sigma*sqrt(T)
call = S*exp(-q*T)*Phi(d1) - K*exp(-r*T)*Phi(d2)
put  = K*exp(-r*T)*Phi(-d2) - S*exp(-q*T)*Phi(-d1)
call - put = S*exp(-q*T) - K*exp(-r*T)
```

The final identity is put-call parity. It provides a check distinct from looking
up one price. Price bounds also give useful checks: call lies between
`max(S*exp(-q*T)-K*exp(-r*T),0)` and `S*exp(-q*T)`; the analogous put bounds
exchange discounted strike and spot.

At `T=0`, return the intrinsic payoff. At zero volatility and positive expiry,
the discounted terminal payoff is deterministic: call is
`max(S*exp(-q*T)-K*exp(-r*T),0)`, and put is its reverse difference.

## Finite tree

With `N` intervals, let `dt=T/N`, `u=exp(sigma*sqrt(dt))`, `d=1/u`, and
`growth=exp((r-q)*dt)`. The up probability is `p=(growth-d)/(u-d)`. This is a
risk-neutral probability fixed by the model, not a forecast of the actual
frequency of price increases.

At terminal node `j`, the underlying is `S*u^(2*j-N)`. Apply the option payoff
there, then repeatedly discount `p*up_value + (1-p)*down_value` by `exp(-r*dt)`.
No early-exercise comparison is performed. Cox, Ross and Rubinstein introduced
this class of discrete approach in
[Option pricing: A simplified approach](https://doi.org/10.1016/0304-405X(79)90015-1).

The chosen grid must produce `p` in `[0,1]`. For insufficient steps or very small
volatility it may not; the implementation reports an error instead of clipping
an invalid probability. Increasing the grid may help, but the supported maximum
does not guarantee a feasible grid for every accepted analytic contract.

## Supported input domain

| Input | Accepted values |
|---|---|
| Spot and strike | 1e-6 to 1e9 |
| Risk-free rate | -0.2 to 0.2 |
| Dividend yield | 0 to 0.2 |
| Volatility | Zero, or 1e-4 to 1 |
| Expiry in years | Zero, or 1e-4 to 10 |
| Tree steps | Integer 1 to 2048 |

All inputs must be finite. Bounds keep this teaching implementation's terminal
prices and work within a deliberate numerical/runtime range. Very small analytic
tail prices can round to zero. Floating-point results are approximations; tests
use tolerances and independent references.

The examples are synthetic and cover dividends and a negative rate. The model
does not include early exercise, discrete cash dividends, stochastic volatility,
jumps, transaction costs, a volatility surface, or parameter estimation.

