#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "option/pricing.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>

namespace {
using option::Contract;
using option::Kind;

double discounted_spot(const Contract& c) { return c.spot * std::exp(-c.dividend_yield * c.maturity); }
double discounted_strike(const Contract& c) { return c.strike * std::exp(-c.rate * c.maturity); }
std::vector<Contract> examples() {
    return {{100,100,.05,0,.2,1}, {100,95,.03,.02,.25,.5},
            {90,110,.02,.01,.3,2}, {110,100,-.01,.01,.2,.75}};
}
void invalid_contract(const Contract& c) {
    CHECK_THROWS_AS(option::validate(c), std::invalid_argument);
    CHECK_THROWS_AS(option::black_scholes(c, Kind::call), std::invalid_argument);
    CHECK_THROWS_AS(option::crr_binomial(c, Kind::put, 8), std::invalid_argument);
}
}

TEST_CASE("standard Black-Scholes call matches independent reference value") {
    CHECK(option::black_scholes({}, Kind::call) == doctest::Approx(10.450583572185565).epsilon(1e-12));
}
TEST_CASE("standard Black-Scholes put matches independent reference value") {
    CHECK(option::black_scholes({}, Kind::put) == doctest::Approx(5.573526022256971).epsilon(1e-12));
}
TEST_CASE("analytic put-call parity includes dividends and negative rates") {
    for (const auto& c : examples()) {
        CHECK(option::black_scholes(c,Kind::call)-option::black_scholes(c,Kind::put) ==
              doctest::Approx(discounted_spot(c)-discounted_strike(c)).epsilon(1e-12));
    }
}
TEST_CASE("tree put-call parity uses the same discounted cash-flow identity") {
    for (const auto& c : examples()) {
        CHECK(option::crr_binomial(c,Kind::call,128)-option::crr_binomial(c,Kind::put,128) ==
              doctest::Approx(discounted_spot(c)-discounted_strike(c)).epsilon(1e-10));
    }
}
TEST_CASE("zero expiry returns intrinsic call and put values") {
    for (double spot : {80.0,100.0,120.0}) {
        Contract c; c.spot=spot; c.maturity=0;
        CHECK(option::black_scholes(c,Kind::call)==std::max(spot-100,0.0));
        CHECK(option::black_scholes(c,Kind::put)==std::max(100-spot,0.0));
        CHECK(option::crr_binomial(c,Kind::call,17)==std::max(spot-100,0.0));
        CHECK(option::crr_binomial(c,Kind::put,17)==std::max(100-spot,0.0));
    }
}
TEST_CASE("zero volatility prices a deterministic discounted payoff") {
    for (auto c : examples()) {
        c.volatility=0;
        const double difference=discounted_spot(c)-discounted_strike(c);
        for (const auto kind : {Kind::call,Kind::put}) {
            const double expected=std::max(kind==Kind::call?difference:-difference,0.0);
            CHECK(option::black_scholes(c,kind)==doctest::Approx(expected).epsilon(1e-12));
            CHECK(option::crr_binomial(c,kind,17)==doctest::Approx(expected).epsilon(1e-12));
        }
    }
}
TEST_CASE("analytic and tree prices satisfy European no-arbitrage bounds") {
    for (const auto& c : examples()) {
        const double a=discounted_spot(c),b=discounted_strike(c);
        for (const auto kind : {Kind::call,Kind::put}) {
            const double lower=std::max(kind==Kind::call?a-b:b-a,0.0);
            const double upper=kind==Kind::call?a:b;
            for (double value : {option::black_scholes(c,kind),option::crr_binomial(c,kind,128)}) {
                CHECK(value>=lower-1e-9); CHECK(value<=upper+1e-9);
            }
        }
    }
}
TEST_CASE("call increases and put decreases with spot") {
    Contract lower;lower.spot=90;Contract higher;higher.spot=110;
    CHECK(option::black_scholes(lower,Kind::call)<option::black_scholes(higher,Kind::call));
    CHECK(option::black_scholes(lower,Kind::put)>option::black_scholes(higher,Kind::put));
    CHECK(option::crr_binomial(lower,Kind::call,128)<option::crr_binomial(higher,Kind::call,128));
    CHECK(option::crr_binomial(lower,Kind::put,128)>option::crr_binomial(higher,Kind::put,128));
}
TEST_CASE("call decreases and put increases with strike") {
    Contract lower;lower.strike=90;Contract higher;higher.strike=110;
    CHECK(option::black_scholes(lower,Kind::call)>option::black_scholes(higher,Kind::call));
    CHECK(option::black_scholes(lower,Kind::put)<option::black_scholes(higher,Kind::put));
    CHECK(option::crr_binomial(lower,Kind::call,128)>option::crr_binomial(higher,Kind::call,128));
    CHECK(option::crr_binomial(lower,Kind::put,128)<option::crr_binomial(higher,Kind::put,128));
}
TEST_CASE("a one-step tree agrees with a two-outcome hand calculation") {
    Contract c;c.rate=std::log(1.05);c.volatility=std::log(1.2);
    const double p=(1.05-1/1.2)/(1.2-1/1.2);
    CHECK(option::crr_binomial(c,Kind::call,1)==doctest::Approx(20*p/1.05).epsilon(1e-12));
    CHECK(option::crr_binomial(c,Kind::put,1)==doctest::Approx((100-100/1.2)*(1-p)/1.05).epsilon(1e-12));
}
TEST_CASE("a two-step tree agrees with direct terminal outcome probabilities") {
    Contract c;c.rate=2*std::log(1.05);c.volatility=std::sqrt(2.0)*std::log(1.2);
    const double p=(1.05-1/1.2)/(1.2-1/1.2);
    CHECK(option::crr_binomial(c,Kind::call,2)==doctest::Approx(44*p*p/(1.05*1.05)).epsilon(1e-12));
    CHECK(option::crr_binomial(c,Kind::put,2)==doctest::Approx((100-100/(1.2*1.2))*(1-p)*(1-p)/(1.05*1.05)).epsilon(1e-12));
}
TEST_CASE("finite-step tree is an approximation rather than exact analytic pricing") {
    const Contract c;
    CHECK(std::abs(option::crr_binomial(c,Kind::call,2)-option::black_scholes(c,Kind::call))>0.1);
}
TEST_CASE("fine trees approximate the four declared synthetic contracts") {
    for (const auto& c : examples()) for (const auto kind : {Kind::call,Kind::put}) {
        const double exact=option::black_scholes(c,kind);
        CHECK(std::abs(option::crr_binomial(c,kind,512)-exact)<0.05);
        CHECK(std::abs(option::crr_binomial(c,kind,1024)-exact)<0.03);
    }
}
TEST_CASE("linear currency scaling preserves both pricing methods") {
    Contract a;Contract b=a;b.spot*=7;b.strike*=7;
    for (const auto kind : {Kind::call,Kind::put}) {
        CHECK(option::black_scholes(b,kind)==doctest::Approx(7*option::black_scholes(a,kind)).epsilon(1e-12));
        CHECK(option::crr_binomial(b,kind,64)==doctest::Approx(7*option::crr_binomial(a,kind,64)).epsilon(1e-11));
    }
}
TEST_CASE("pricing preserves every contract field") {
    const Contract c{90,110,.02,.01,.3,2};
    option::black_scholes(c,Kind::call);option::crr_binomial(c,Kind::put,32);
    CHECK(c.spot==90);CHECK(c.strike==110);CHECK(c.rate==.02);
    CHECK(c.dividend_yield==.01);CHECK(c.volatility==.3);CHECK(c.maturity==2);
}
TEST_CASE("input domain endpoints are accepted") {
    Contract c;c.spot=1e-6;c.strike=1e9;c.rate=-.2;c.dividend_yield=.2;c.volatility=1;c.maturity=10;
    CHECK_NOTHROW(option::validate(c));CHECK(std::isfinite(option::black_scholes(c,Kind::put)));
    c.spot=1e9;c.strike=1e-6;c.rate=.2;c.dividend_yield=0;c.volatility=1e-4;c.maturity=1e-4;
    CHECK_NOTHROW(option::validate(c));CHECK(std::isfinite(option::black_scholes(c,Kind::call)));
}
TEST_CASE("invalid spot and strike ranges are rejected") {
    for (double value : {-1.0,0.0,1e-7,1e10}) {
        Contract c;c.spot=value;invalid_contract(c);c={};c.strike=value;invalid_contract(c);
    }
}
TEST_CASE("nonfinite values in every field are rejected") {
    for (double value : {std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity(),-std::numeric_limits<double>::infinity()}) {
        for (int field=0;field<6;++field) {
            Contract c;
            switch(field) {case 0:c.spot=value;break;case 1:c.strike=value;break;case 2:c.rate=value;break;case 3:c.dividend_yield=value;break;case 4:c.volatility=value;break;default:c.maturity=value;}
            invalid_contract(c);
        }
    }
}
TEST_CASE("risk-free rate and dividend domain violations are rejected") {
    for (double value : {-.21,.21}) {Contract c;c.rate=value;invalid_contract(c);}
    for (double value : {-.01,.21}) {Contract c;c.dividend_yield=value;invalid_contract(c);}
}
TEST_CASE("volatility admits zero but rejects the excluded positive gap") {
    for (double value : {-.1,1e-5,1.01}) {Contract c;c.volatility=value;invalid_contract(c);}
}
TEST_CASE("maturity admits zero but rejects the excluded positive gap") {
    for (double value : {-1.0,1e-5,10.01}) {Contract c;c.maturity=value;invalid_contract(c);}
}
TEST_CASE("invalid option enum values are rejected including degenerate contracts") {
    for (double maturity : {0.0,1.0}) {
        Contract c;c.maturity=maturity;
        CHECK_THROWS_AS(option::black_scholes(c,static_cast<Kind>(99)),std::invalid_argument);
        CHECK_THROWS_AS(option::crr_binomial(c,static_cast<Kind>(99),32),std::invalid_argument);
    }
}
TEST_CASE("tree step bounds apply even at zero expiry or volatility") {
    for (std::size_t steps : {std::size_t(0),std::size_t(2049),std::numeric_limits<std::size_t>::max()}) {
        for (int degenerate=0;degenerate<3;++degenerate) {
            Contract c;if(degenerate==1)c.maturity=0;if(degenerate==2)c.volatility=0;
            CHECK_THROWS_AS(option::crr_binomial(c,Kind::call,steps),std::invalid_argument);
        }
    }
}
TEST_CASE("inadmissible risk-neutral probabilities are errors rather than clipped guesses") {
    Contract c;c.rate=.2;c.volatility=1e-4;
    CHECK_THROWS_AS(option::crr_binomial(c,Kind::call,1),std::invalid_argument);
    c.rate=-.2;c.dividend_yield=.2;
    CHECK_THROWS_AS(option::crr_binomial(c,Kind::put,1),std::invalid_argument);
}
TEST_CASE("the largest supported tree remains finite on an ordinary contract") {
    CHECK(std::isfinite(option::crr_binomial({},Kind::call,2048)));
}
