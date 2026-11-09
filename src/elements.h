#pragma once

// Circuit elements. Each element is a branch between strnode and stpnode
// (node 0 = ground), represented for every time step by an equivalent
// resistance Reff in parallel with a history current source ihistory:
//
//     i = V / Reff + ihistory,   V = V(strnode) - V(stpnode),
//     i flows from strnode to stpnode.

#include <optional>
#include <vector>

enum class BranchType { R, L, C, Source, Switch, Fault };

class Element {
public:
    Element(BranchType type, int strnode, int stpnode) : brnType(type), strnode(strnode), stpnode(stpnode) {}
    virtual ~Element() = default;

    // History current for the coming time step (resistive branches: 0)
    virtual void calcHistory() { ihistory = 0.0; }

    // Ideal voltage sources have no conductance; they fix a known node voltage
    virtual bool isIdealSource() const { return false; }

    BranchType brnType;
    int strnode, stpnode;
    double Reff = 0.0;
    double Ilast = 0.0;     // branch current at the previous time step
    double Vlast = 0.0;     // branch voltage at the previous time step
    double ihistory = 0.0;
    int fi = -1, ti = -1;   // internal matrix indices of strnode / stpnode (-1 = ground)
};

// Resistor, inductor or capacitor with trapezoidal-rule companion model
class RLC : public Element {
public:
    // value in ohm (R), henry (L) or farad (C)
    RLC(BranchType type, int strnode, int stpnode, double value);
    void calcHistory() override;
};

// Sinusoidal voltage source: E(t) = magnitude * cos(2*pi*frequency*t + angle),
// raising stpnode above strnode by E.
//   resistance > 0  : Norton equivalent (E/R current source in parallel with R)
//   resistance <= 0 : ideal voltage source; one terminal must be ground and
//                     the other node becomes a known node
class Source : public Element {
public:
    Source(int strnode, int stpnode, double magnitude, double angle_deg, double frequency,
           double resistance = 0.0);

    void update(double t);   // compute E at time t
    void calcHistory() override;
    bool isIdealSource() const override { return ideal; }

    double magnitude, angle, frequency;
    bool ideal;
    double E = 0.0;
};

// Time-controlled switch (breaker): Ron when closed, Roff when open.
// It starts in state `closed` and toggles at each time in t_switch.
// Switching is instantaneous at the given time (no waiting for a current zero).
class Switch : public Element {
public:
    Switch(int strnode, int stpnode, bool closed, std::vector<double> t_switch,
           double Ron = 0.01, double Roff = 1.0e6);

    bool stateAt(double t) const;   // true if closed at time t
    bool update(double t);          // set state for time t; true if it changed

    std::vector<double> t_switch;
    double Ron, Roff;
    bool closed0, closed;

protected:
    Switch(BranchType type, int strnode, int stpnode, bool closed, std::vector<double> t_switch,
           double Ron, double Roff);
    virtual const char* name() const { return "switch"; }
    virtual const char* describe(bool isClosed) const { return isClosed ? "closed" : "opened"; }
};

// Fault: a switch that is closed from t_apply until t_clear
// (no t_clear = permanent fault)
class Fault : public Switch {
public:
    Fault(int strnode, int stpnode, double t_apply, std::optional<double> t_clear,
          double Ron = 0.01, double Roff = 1.0e6);

    double t_apply;
    std::optional<double> t_clear;

protected:
    const char* name() const override { return "fault"; }
    const char* describe(bool isClosed) const override { return isClosed ? "applied" : "cleared"; }
};
