"""Independent Decimal references: integrated normal density and terminal-tree sums."""
from decimal import Decimal, localcontext
from math import comb

PRECISION = 80
PI = Decimal("3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986")
FIELDS = ("spot", "strike", "rate", "dividend_yield", "volatility", "maturity")


def values(contract):
    return tuple(Decimal(str(contract[name])) for name in FIELDS)


def normal_cdf(x):
    """Integrate exp(-t*t/2) by its Taylor series, not the C/C++ erf routine."""
    if abs(x) > 8:
        raise ValueError("reference series is restricted to |x| <= 8")
    term = x
    integral = term
    for n in range(1, 1000):
        term *= -x * x / Decimal(2 * n)
        increment = term / Decimal(2 * n + 1)
        integral += increment
        if abs(increment) < Decimal("1e-75"):
            return Decimal("0.5") + integral / (2 * PI).sqrt()
    raise ArithmeticError("normal-density reference did not converge")


def black_scholes(contract, kind):
    with localcontext() as context:
        context.prec = PRECISION
        spot, strike, rate, dividend, volatility, maturity = values(contract)
        sign = Decimal(1 if kind == "call" else -1)
        if maturity == 0:
            return max(sign * (spot - strike), Decimal(0))
        a = spot * (-dividend * maturity).exp()
        b = strike * (-rate * maturity).exp()
        if volatility == 0:
            return max(sign * (a - b), Decimal(0))
        sigma_t = volatility * maturity.sqrt()
        d1 = ((spot / strike).ln() + (rate - dividend + volatility * volatility / 2) * maturity) / sigma_t
        d2 = d1 - sigma_t
        if kind == "call":
            return a * normal_cdf(d1) - b * normal_cdf(d2)
        return b * normal_cdf(-d2) - a * normal_cdf(-d1)


def terminal_tree(contract, kind, steps):
    """Direct terminal binomial expectation, independent of backward induction."""
    with localcontext() as context:
        context.prec = PRECISION
        spot, strike, rate, dividend, volatility, maturity = values(contract)
        if maturity == 0 or volatility == 0:
            return black_scholes(contract, kind)
        dt = maturity / steps
        up = (volatility * dt.sqrt()).exp()
        down = 1 / up
        probability = (((rate - dividend) * dt).exp() - down) / (up - down)
        if not 0 <= probability <= 1:
            raise ValueError("inadmissible reference tree")
        total = Decimal(0)
        for up_count in range(steps + 1):
            terminal = spot * up ** (2 * up_count - steps)
            payoff = max(terminal - strike if kind == "call" else strike - terminal, Decimal(0))
            up_mass = probability ** up_count if up_count else Decimal(1)
            down_count = steps - up_count
            down_mass = (1 - probability) ** down_count if down_count else Decimal(1)
            total += Decimal(comb(steps, up_count)) * up_mass * down_mass * payoff
        return (-rate * maturity).exp() * total
