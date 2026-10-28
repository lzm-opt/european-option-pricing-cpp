#include "option/pricing.hpp"

#include <charconv>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <locale>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

void help() {
    std::cout <<
        "European option prices: Black-Scholes and Cox-Ross-Rubinstein trees.\n"
        "Usage: option_pricer [option value ...]\n\n"
        "  --spot NUMBER          Spot price, 1e-6..1e9 (default 100)\n"
        "  --strike NUMBER        Strike price, 1e-6..1e9 (default 100)\n"
        "  --rate RATE            Continuous annual rate, -0.2..0.2 (default 0.05)\n"
        "  --dividend RATE        Continuous annual dividend yield, 0..0.2 (default 0)\n"
        "  --volatility NUMBER    Annual volatility, zero or 1e-4..1 (default 0.2)\n"
        "  --maturity YEARS       Time to expiry, zero or 1e-4..10 (default 1)\n"
        "  --kind NAME            call or put (default call)\n"
        "  --method NAME          black-scholes or binomial (default black-scholes)\n"
        "  --steps INTEGER        Tree intervals, 1..2048; binomial only (default 512)\n"
        "  --help                 Show this help; use it alone\n\n"
        "Rates and volatility are decimals: 0.05 means 5%, and 0.2 means 20%.\n"
        "All rates use continuous compounding; maturity is measured in years.\n"
        "Only European exercise is implemented. Zero expiry gives intrinsic value;\n"
        "zero volatility gives the discounted deterministic payoff. A finite tree\n"
        "generally differs from Black-Scholes and needs a valid probability in [0,1].\n"
        "Output is metric,value CSV on standard output. Analytic output reports\n"
        "steps=0; errors go to standard error and exit with status 1.\n";
}

bool known(const std::string& option) {
    return option == "--spot" || option == "--strike" || option == "--rate" ||
           option == "--dividend" || option == "--volatility" || option == "--maturity" ||
           option == "--kind" || option == "--method" || option == "--steps";
}

bool decimal_grammar(const std::string& value) {
    std::size_t index = 0;
    if (!value.empty() && (value[0] == '+' || value[0] == '-')) ++index;
    std::size_t digits = 0;
    while (index < value.size() && value[index] >= '0' && value[index] <= '9') {
        ++index;
        ++digits;
    }
    if (index < value.size() && value[index] == '.') {
        ++index;
        while (index < value.size() && value[index] >= '0' && value[index] <= '9') {
            ++index;
            ++digits;
        }
    }
    if (digits == 0) return false;
    if (index < value.size() && (value[index] == 'e' || value[index] == 'E')) {
        ++index;
        if (index < value.size() && (value[index] == '+' || value[index] == '-')) ++index;
        const std::size_t exponent_start = index;
        while (index < value.size() && value[index] >= '0' && value[index] <= '9') ++index;
        if (index == exponent_start) return false;
    }
    return index == value.size();
}

double number(const std::string& value, const std::string& name) {
    if (!decimal_grammar(value)) {
        throw std::invalid_argument(name + " requires a finite decimal number");
    }
    std::istringstream input(value);
    input.imbue(std::locale::classic());
    input >> std::noskipws;
    double result = 0.0;
    if (!(input >> result) || input.peek() != std::char_traits<char>::eof() ||
        !std::isfinite(result)) {
        throw std::invalid_argument(name + " requires a finite decimal number");
    }
    return result;
}

std::size_t count(const std::string& value) {
    if (value.empty()) throw std::invalid_argument("--steps requires a positive integer");
    for (char digit : value) {
        if (digit < '0' || digit > '9') {
            throw std::invalid_argument("--steps accepts decimal digits only");
        }
    }
    std::size_t result = 0;
    const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);
    if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size() ||
        result == 0 || result > 2048) {
        throw std::invalid_argument("--steps must be between 1 and 2048");
    }
    return result;
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string(argv[1]) == "--help") {
            help();
            return 0;
        }
        std::map<std::string, std::string> options;
        for (int index = 1; index < argc; index += 2) {
            const std::string option(argv[index]);
            if (!known(option)) throw std::invalid_argument("unknown option: " + option);
            if (index + 1 == argc) throw std::invalid_argument("missing value for " + option);
            if (!options.emplace(option, argv[index + 1]).second) {
                throw std::invalid_argument("duplicate option: " + option);
            }
        }
        auto numeric = [&options](const char* key, double fallback) {
            const auto found = options.find(key);
            return found == options.end() ? fallback : number(found->second, key);
        };
        option::Contract contract;
        contract.spot = numeric("--spot", contract.spot);
        contract.strike = numeric("--strike", contract.strike);
        contract.rate = numeric("--rate", contract.rate);
        contract.dividend_yield = numeric("--dividend", contract.dividend_yield);
        contract.volatility = numeric("--volatility", contract.volatility);
        contract.maturity = numeric("--maturity", contract.maturity);
        const std::string kind_name = options.count("--kind") != 0 ? options.at("--kind") : "call";
        option::Kind kind = option::Kind::call;
        if (kind_name == "put") kind = option::Kind::put;
        else if (kind_name != "call") throw std::invalid_argument("--kind must be call or put");
        const std::string method = options.count("--method") != 0
            ? options.at("--method") : "black-scholes";
        if (method != "black-scholes" && method != "binomial") {
            throw std::invalid_argument("--method must be black-scholes or binomial");
        }
        if (method == "black-scholes" && options.count("--steps") != 0) {
            throw std::invalid_argument("--steps is only accepted with --method binomial");
        }
        const std::size_t steps = method == "binomial"
            ? (options.count("--steps") != 0 ? count(options.at("--steps")) : 512) : 0;
        const double price = method == "black-scholes" ? option::black_scholes(contract, kind)
                                                        : option::crr_binomial(contract, kind, steps);
        std::cout.imbue(std::locale::classic());
        std::cout << std::setprecision(17)
                  << "metric,value\nmethod," << method
                  << "\nkind," << kind_name
                  << "\nprice," << price
                  << "\nsteps," << steps << '\n';
        if (!std::cout) throw std::runtime_error("failed to write CSV output");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << "\nUse --help for usage.\n";
        return 1;
    }
}
