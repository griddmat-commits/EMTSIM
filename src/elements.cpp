#include "elements.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <stdexcept>

#include "config.h"

namespace {
constexpr double PI = 3.14159265358979323846;
}

// ---------------------------------------------------------------- RLC

RLC::RLC(BranchType type, int strnode, int stpnode, double value) : Element(type, strnode, stpnode) {
    switch (type) {
        case BranchType::R: Reff = value; break;
        case BranchType::L: Reff = 2.0 * value / config::Dt; break;   // 2L/Dt
        case BranchType::C: Reff = config::Dt / (2.0 * value); break; // Dt/2C
        default: throw std::invalid_argument("RLC: type must be R, L or C");
    }
}

void RLC::calcHistory() {
    switch (brnType) {
        case BranchType::L: ihistory = Ilast + Vlast / Reff; break;
        case BranchType::C: ihistory = -Ilast - Vlast / Reff; break;
        default: ihistory = 0.0; break;
    }
}

// ---------------------------------------------------------------- Source

Source::Source(int strnode, int stpnode, double magnitude, double angle_deg, double frequency,
               double resistance)
    : Element(BranchType::Source, strnode, stpnode),
      magnitude(magnitude), angle(angle_deg), frequency(frequency), ideal(resistance <= 0.0) {
    Reff = ideal ? 0.0 : resistance;
}

void Source::update(double t) {
    E = magnitude * std::cos(2.0 * PI * frequency * t + angle * PI / 180.0);
}

void Source::calcHistory() {
    // Norton equivalent: current source E/R in parallel with R
    ihistory = ideal ? 0.0 : E / Reff;
}

// ---------------------------------------------------------------- Switch

Switch::Switch(int strnode, int stpnode, bool closed, std::vector<double> t_switch, double Ron, double Roff)
    : Switch(BranchType::Switch, strnode, stpnode, closed, std::move(t_switch), Ron, Roff) {}

Switch::Switch(BranchType type, int strnode, int stpnode, bool closed, std::vector<double> times,
               double Ron, double Roff)
    : Element(type, strnode, stpnode), t_switch(std::move(times)), Ron(Ron), Roff(Roff),
      closed0(closed), closed(closed) {
    std::sort(t_switch.begin(), t_switch.end());
    Reff = closed ? Ron : Roff;
}

bool Switch::stateAt(double t) const {
    // Small tolerance: time is accumulated as Time += Dt, so e.g. 200*Dt can land
    // just below 0.02 and the switch would otherwise operate one step late.
    const double tol = 1e-6 * config::Dt;
    const auto n = std::count_if(t_switch.begin(), t_switch.end(), [&](double ts) { return t >= ts - tol; });
    return closed0 != (n % 2 == 1);
}

bool Switch::update(double t) {
    const bool c = stateAt(t);
    if (c == closed) return false;
    closed = c;
    Reff = closed ? Ron : Roff;
    std::printf("t = %.5f s: %s %s (nodes %d-%d)\n", t, name(), describe(closed), strnode, stpnode);
    return true;
}

// ---------------------------------------------------------------- Fault

namespace {
std::vector<double> faultTimes(double t_apply, std::optional<double> t_clear) {
    if (t_clear) return {t_apply, *t_clear};
    return {t_apply};
}
}  // namespace

Fault::Fault(int strnode, int stpnode, double t_apply, std::optional<double> t_clear, double Ron, double Roff)
    : Switch(BranchType::Fault, strnode, stpnode, false, faultTimes(t_apply, t_clear), Ron, Roff),
      t_apply(t_apply), t_clear(t_clear) {}
