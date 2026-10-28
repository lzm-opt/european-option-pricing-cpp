/** @file pricing.hpp
 * @brief European option prices with analytic and recombining-tree methods.
 */
#pragma once
#include <cstddef>

namespace option {

/** European exercise payoff at maturity. */
enum class Kind { call, put };

/** Constant-parameter model with continuous dividend yield and compounding. */
struct Contract {
    double spot{100.0}; ///< Spot in currency units, from 1e-6 to 1e9.
    double strike{100.0}; ///< Strike in currency units, from 1e-6 to 1e9.
    double rate{0.05}; ///< Continuous annual risk-free rate, from -0.2 to 0.2.
    double dividend_yield{0.0}; ///< Continuous annual dividend yield, from 0 to 0.2.
    double volatility{0.2}; ///< Annual volatility: zero, or from 1e-4 to 1.
    double maturity{1.0}; ///< Years to expiry: zero, or from 1e-4 to 10.
};

/** Validate all model inputs, including finiteness and documented bounds.
 * @param contract Model inputs in the units stated above.
 * @throws std::invalid_argument For inputs outside the supported domain.
 */
void validate(const Contract& contract);

/** Black-Scholes price with continuous dividend yield.
 * @param contract Model inputs; zero expiry returns intrinsic value.
 * @param kind Call or put. Zero volatility uses the deterministic payoff.
 * @return A nonnegative price in currency units.
 * @throws std::invalid_argument For invalid contracts or option kinds.
 * @throws std::overflow_error If numerical evaluation is not finite.
 * @note Constant work and storage. Underflow of a negligible tail price may give zero.
 */
double black_scholes(const Contract& contract, Kind kind);

/** European CRR price by backward induction, without early exercise.
 * @param contract Model inputs, with the same zero-expiry/volatility limits.
 * @param kind Call or put.
 * @param steps Number of tree intervals, from 1 to 2048.
 * @return A finite-step approximation in currency units.
 * @throws std::invalid_argument For invalid inputs or a risk-neutral probability
 *         outside [0,1]; some contracts need more steps than the supported maximum.
 * @throws std::overflow_error If numerical evaluation is not finite.
 * @note Time O(steps squared), storage O(steps). Finite-step price is generally
 *       different from the analytic price; convergence can oscillate.
 */
double crr_binomial(const Contract& contract, Kind kind, std::size_t steps = 512);

} // namespace option
