#include "option/pricing.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>

namespace option {
namespace {

void check_kind(Kind kind) {
    if (kind != Kind::call && kind != Kind::put) {
        throw std::invalid_argument("option kind must be call or put");
    }
}

bool bounded(double value, double lower, double upper) {
    return std::isfinite(value) && value >= lower && value <= upper;
}

long double normal_cdf(long double value) {
    return 0.5L * std::erfc(-value / std::sqrt(2.0L));
}

long double payoff(long double spot, long double strike, Kind kind) {
    return std::max(kind == Kind::call ? spot - strike : strike - spot, 0.0L);
}

double finite_price(long double value) {
    if (!std::isfinite(value) || value < 0.0L ||
        value > std::numeric_limits<double>::max()) {
        throw std::overflow_error("option price is not a nonnegative finite double");
    }
    return static_cast<double>(value);
}

// Only roundoff-sized departures from the analytic no-arbitrage bounds are
// corrected. A material violation is reported instead of being hidden.
double analytic_output(long double value, long double discounted_spot,
                       long double discounted_strike, Kind kind) {
    const long double lower = payoff(discounted_spot, discounted_strike, kind);
    const long double upper = kind == Kind::call ? discounted_spot : discounted_strike;
    const long double tolerance = 128.0L * std::numeric_limits<long double>::epsilon() *
                                  std::max(1.0L, upper);
    if (!std::isfinite(value) || value < lower - tolerance || value > upper + tolerance) {
        throw std::overflow_error("analytic price exceeds its numerical no-arbitrage bounds");
    }
    return finite_price(std::clamp(value, lower, upper));
}

long double discounted_payoff(const Contract& contract, Kind kind) {
    const long double time = contract.maturity;
    const long double discounted_spot = static_cast<long double>(contract.spot) *
                                        std::exp(-contract.dividend_yield * time);
    const long double discounted_strike = static_cast<long double>(contract.strike) *
                                          std::exp(-contract.rate * time);
    return payoff(discounted_spot, discounted_strike, kind);
}

} // namespace

void validate(const Contract& contract) {
    if (!bounded(contract.spot, 1e-6, 1e9) || !bounded(contract.strike, 1e-6, 1e9)) {
        throw std::invalid_argument("spot and strike must be finite and between 1e-6 and 1e9");
    }
    if (!bounded(contract.rate, -0.2, 0.2)) {
        throw std::invalid_argument("annual rate must be finite and between -0.2 and 0.2");
    }
    if (!bounded(contract.dividend_yield, 0.0, 0.2)) {
        throw std::invalid_argument("annual dividend yield must be finite and between 0 and 0.2");
    }
    if (!(contract.volatility == 0.0 || bounded(contract.volatility, 1e-4, 1.0))) {
        throw std::invalid_argument("volatility must be zero or finite and between 1e-4 and 1");
    }
    if (!(contract.maturity == 0.0 || bounded(contract.maturity, 1e-4, 10.0))) {
        throw std::invalid_argument("maturity must be zero or finite and between 1e-4 and 10 years");
    }
}

double black_scholes(const Contract& contract, Kind kind) {
    validate(contract);
    check_kind(kind);
    if (contract.maturity == 0.0) {
        return finite_price(payoff(contract.spot, contract.strike, kind));
    }
    if (contract.volatility == 0.0) {
        return finite_price(discounted_payoff(contract, kind));
    }
    const long double time = contract.maturity;
    const long double volatility = contract.volatility;
    const long double width = volatility * std::sqrt(time);
    const long double d1 = (std::log(static_cast<long double>(contract.spot)) -
                            std::log(static_cast<long double>(contract.strike)) +
                            (static_cast<long double>(contract.rate) - contract.dividend_yield +
                             0.5L * volatility * volatility) * time) / width;
    const long double d2 = d1 - width;
    const long double discounted_spot = static_cast<long double>(contract.spot) *
                                        std::exp(-contract.dividend_yield * time);
    const long double discounted_strike = static_cast<long double>(contract.strike) *
                                          std::exp(-contract.rate * time);
    long double call = 0.0L;
    long double put = 0.0L;
    // Evaluate the out-of-the-money side first, then use put-call parity. This
    // avoids obtaining a small time value by subtracting large intrinsic values.
    if (std::max(std::abs(d1), std::abs(d2)) < 0.5L) {
        // Near forward at-the-money, erf avoids subtracting two values near
        // one half. In particular, a tiny-volatility time value stays visible.
        const long double root_two = std::sqrt(2.0L);
        const long double shared = 0.5L *
            (discounted_spot * std::erf(d1 / root_two) -
             discounted_strike * std::erf(d2 / root_two));
        const long double spread = discounted_spot - discounted_strike;
        call = shared + 0.5L * spread;
        put = shared - 0.5L * spread;
    } else if (discounted_spot >= discounted_strike) {
        put = discounted_strike * normal_cdf(-d2) - discounted_spot * normal_cdf(-d1);
        put = std::max(put, 0.0L);
        call = put + (discounted_spot - discounted_strike);
    } else {
        call = discounted_spot * normal_cdf(d1) - discounted_strike * normal_cdf(d2);
        call = std::max(call, 0.0L);
        put = call + (discounted_strike - discounted_spot);
    }
    return analytic_output(kind == Kind::call ? call : put,
                           discounted_spot, discounted_strike, kind);
}

double crr_binomial(const Contract& contract, Kind kind, std::size_t steps) {
    validate(contract);
    check_kind(kind);
    if (steps == 0 || steps > 2048) {
        throw std::invalid_argument("tree steps must be between 1 and 2048");
    }
    if (contract.maturity == 0.0) {
        return finite_price(payoff(contract.spot, contract.strike, kind));
    }
    if (contract.volatility == 0.0) {
        return finite_price(discounted_payoff(contract, kind));
    }
    const long double dt = static_cast<long double>(contract.maturity) / steps;
    const long double jump = static_cast<long double>(contract.volatility) * std::sqrt(dt);
    const long double carry = (static_cast<long double>(contract.rate) -
                               contract.dividend_yield) * dt;
    const long double down_change = std::expm1(-jump);
    const long double probability = (std::expm1(carry) - down_change) /
                                    (std::expm1(jump) - down_change);
    if (!std::isfinite(probability) || probability < 0.0L || probability > 1.0L) {
        throw std::invalid_argument("CRR probability is outside [0,1]; try more steps within the supported limit");
    }
    const long double discount = std::exp(-static_cast<long double>(contract.rate) * dt);
    std::vector<long double> values(steps + 1);
    for (std::size_t up_moves = 0; up_moves <= steps; ++up_moves) {
        const long double exponent = (2.0L * up_moves - static_cast<long double>(steps)) * jump;
        const long double terminal_spot = static_cast<long double>(contract.spot) * std::exp(exponent);
        values[up_moves] = payoff(terminal_spot, contract.strike, kind);
    }
    for (std::size_t level = steps; level > 0; --level) {
        for (std::size_t node = 0; node < level; ++node) {
            values[node] = discount * (probability * values[node + 1] +
                                       (1.0L - probability) * values[node]);
        }
    }
    return finite_price(values[0]);
}

} // namespace option
