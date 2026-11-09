// Checks the solver against analytical solutions of first-order circuits.
// Run with:  ctest --test-dir build   (or run build/test_emtsim directly)

#include <cmath>
#include <complex>
#include <cstdio>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "circuit.h"
#include "config.h"
#include "linalg.h"
#include "netlist.h"
#include "network.h"

#include <algorithm>
#include <fstream>

namespace {

using cd = std::complex<double>;
constexpr double PI = 3.14159265358979323846;
constexpr double Vm = 100.0, f = 50.0, a_deg = -90.0;  // E(0) = 0
const double w = 2 * PI * f;
const cd ph = std::polar(1.0, a_deg * PI / 180.0);
const cd j(0.0, 1.0);

double E(double t) { return Vm * std::cos(w * t + a_deg * PI / 180.0); }

// Voltage across L in a series RL circuit energised at t = 0
double rl_vL(double t, double R, double L) {
    const cd I = Vm * ph / (R + j * w * L);
    const double i = std::real(I * std::exp(j * w * t)) - std::real(I) * std::exp(-R * t / L);
    return E(t) - R * i;
}

// Voltage across C in a series RC circuit energised at t = 0
double rc_vC(double t, double R, double C) {
    const cd Zc = 1.0 / (j * w * C);
    const cd Vc = Vm * ph * Zc / (R + Zc);
    return std::real(Vc * std::exp(j * w * t)) - std::real(Vc) * std::exp(-t / (R * C));
}

struct Run {
    std::vector<double> t;
    std::vector<double> v;
};

// Same time loop as main.cpp, recording one node, for a fixed number of steps
Run simulate(Circuit ckt, int node, int steps = 1000) {
    std::ostringstream quiet;
    auto* old = std::cout.rdbuf(quiet.rdbuf());   // silence matrix/event printing
    Network net(std::move(ckt.elements));
    double t = config::srtTime;
    auto refactor = [&] { net.formG(); net.Gdivide(); net.LU(); };
    net.updateSwitches(t);
    refactor();
    Run r;
    for (int s = 0; s < steps; ++s) {
        net.Vdivide();
        t += config::Dt;
        if (net.updateSwitches(t)) refactor();
        net.Vkupdate(t); net.calcBrnHistory(); net.calcinjection(); net.Idivide();
        net.calIUnew(); net.calcnewV(); net.reconstruct_V(); net.calcNewbranchI();
        r.t.push_back(t);
        r.v.push_back(net.nodeVoltage(node));
    }
    std::cout.rdbuf(old);
    return r;
}

int failures = 0;

void check(bool ok, const std::string& what, double value) {
    std::printf("%s  %-58s %.2e\n", ok ? "PASS" : "FAIL", what.c_str(), value);
    if (!ok) ++failures;
}

double maxErr(const Run& r, const std::function<double(double)>& exact, double tmin = 0.0) {
    double e = 0.0;
    for (std::size_t i = 0; i < r.t.size(); ++i)
        if (r.t[i] > tmin) e = std::max(e, std::fabs(r.v[i] - exact(r.t[i])) / Vm);
    return e;
}

}  // namespace

int main() {
    const double R = 2.0, L = 10e-3, C = 2e-3, tol = 1e-3;

    {
        auto mk = [&] { Circuit k; k.addSource(0, 1, Vm, a_deg, f); k.addR(1, 2, R); k.addL(0, 2, L); return k; };
        const double e = maxErr(simulate(mk(), 2), [&](double t) { return rl_vL(t, R, L); });
        check(e < tol, "RL, ideal source, L ground->node (max err pu)", e);
    }
    {
        auto mk = [&] { Circuit k; k.addSource(0, 1, Vm, a_deg, f, R); k.addL(1, 0, L); return k; };
        const double e = maxErr(simulate(mk(), 1), [&](double t) { return rl_vL(t, R, L); });
        check(e < tol, "RL, Norton source, L node->ground (max err pu)", e);
    }
    {
        auto mk = [&] { Circuit k; k.addSource(0, 2, Vm, a_deg, f); k.addR(2, 1, R); k.addC(0, 1, C); return k; };
        const double e = maxErr(simulate(mk(), 1), [&](double t) { return rc_vC(t, R, C); });
        check(e < tol, "RC, ideal source on node 2, C ground->node (max err pu)", e);
    }
    {
        auto mk = [&] { Circuit k; k.addSource(1, 0, Vm, a_deg, f); k.addR(1, 2, R); k.addC(2, 0, C); return k; };
        const double e = maxErr(simulate(mk(), 2), [&](double t) { return -rc_vC(t, R, C); });
        check(e < tol, "RC, ideal source reversed (1->0) (max err pu)", e);
    }
    {
        auto mk = [&] { Circuit k; k.addSource(0, 1, Vm, a_deg, f, R); k.addC(0, 1, C); return k; };
        const double e = maxErr(simulate(mk(), 1), [&](double t) { return rc_vC(t, R, C); });
        check(e < tol, "RC, Norton source (max err pu)", e);
    }
    {
        // Switch closing at 20 ms into a series RL circuit
        const double Ron = 0.01, tsw = 0.02, Rt = R + Ron;
        auto mk = [&] {
            Circuit k;
            k.addSource(0, 1, Vm, a_deg, f); k.addSwitch(1, 2, false, {tsw}, Ron);
            k.addR(2, 3, R); k.addL(3, 0, L);
            return k;
        };
        const Run r = simulate(mk(), 3);
        auto exact = [&](double t) {
            const double tp = t - tsw;
            if (tp < 0) return 0.0;
            const cd I = Vm * std::polar(1.0, w * tsw + a_deg * PI / 180.0) / (Rt + j * w * L);
            const double i = std::real(I * std::exp(j * w * tp)) - std::real(I) * std::exp(-Rt * tp / L);
            return E(t) - Rt * i;
        };
        const double e = maxErr(r, exact, tsw + 0.5 * config::Dt);
        check(e < tol, "switch closing into RL at 20 ms (max err pu)", e);
        double pre = 0.0;
        for (std::size_t i = 0; i < r.t.size(); ++i)
            if (r.t[i] < tsw - 0.5 * config::Dt) pre = std::max(pre, std::fabs(r.v[i]) / Vm);
        check(pre < 1e-3, "node isolated while switch open (max |V| pu)", pre);
    }
    {
        Switch sw(1, 2, true, {0.08, 0.03});
        const bool ok = sw.stateAt(0.0) && sw.stateAt(0.0299) && !sw.stateAt(0.03) && !sw.stateAt(0.05) &&
                        sw.stateAt(0.08) && sw.stateAt(0.09);
        check(ok, "switch toggle sequence: open @30 ms, reclose @80 ms", 0.0);
        Fault fl(1, 0, 0.02, 0.04);
        const bool okf = !fl.stateAt(0.0199) && fl.stateAt(0.02) && fl.stateAt(0.0399) && !fl.stateAt(0.04);
        check(okf, "fault applied @20 ms, cleared @40 ms", 0.0);
    }
    {
        // Norton source with internal R == ideal source + separate series R
        auto net = [](Circuit k) {
            k.addL(3, 2, 1e-3); k.addC(2, 1, 1e-3); k.addR(0, 2, 1e-3); k.addL(0, 1, 1e-3);
            return k;
        };
        Circuit a; a.addSource(0, 3, 100, 45, 100, 1.0);
        Circuit b; b.addSource(0, 4, 100, 45, 100); b.addR(4, 3, 1.0);
        const Run ra = simulate(net(std::move(a)), 2), rb = simulate(net(std::move(b)), 2);
        double d = 0.0;
        for (std::size_t i = 0; i < ra.v.size(); ++i) d = std::max(d, std::fabs(ra.v[i] - rb.v[i]));
        check(d < 1e-9, "Norton source == ideal source + series R (max diff V)", d);
    }

    {
        // Sparse LDL^T against dense LU on conductance-like matrices: a meshed grid of
        // branches with a range of conductances (1e-6 .. 1e3 S) and some branches to ground
        auto build = [](std::size_t n, unsigned seed) {
            SparseMatrix A(n, n);
            unsigned s = seed;
            auto rnd = [&s] { s = s * 1103515245u + 12345u; return (s >> 8) / double(1u << 24); };
            auto branch = [&](std::size_t p, std::size_t q, double g) {
                A.add(p, p, g); A.add(q, q, g); A.add(p, q, -g); A.add(q, p, -g);
            };
            const std::size_t width = 10;
            for (std::size_t i = 0; i < n; ++i) {
                const double g = std::pow(10.0, -6.0 + 9.0 * rnd());
                if (i + 1 < n && (i + 1) % width) branch(i, i + 1, g);
                if (i + width < n) branch(i, i + width, std::pow(10.0, -2.0 + 4.0 * rnd()));
                if (i % 7 == 0) A.add(i, i, 0.5 + rnd());   // to ground
            }
            A.finalize();
            return A;
        };
        double worst = 0.0;
        for (std::size_t n : {1u, 2u, 17u, 120u, 400u}) {
            const SparseMatrix A = build(n, 7u + unsigned(n));
            Vector b(n);
            for (std::size_t i = 0; i < n; ++i) b[i] = std::sin(1.0 + double(i));
            SparseLDL ldl; ldl.factor(A);
            LUFactor lu; lu.factor(A.toDense());
            const Vector xs = ldl.solve(b), xd = lu.solve(b);
            double num = 0.0, den = 0.0;
            for (std::size_t i = 0; i < n; ++i) { num = std::max(num, std::fabs(xs[i] - xd[i])); den = std::max(den, std::fabs(xd[i])); }
            worst = std::max(worst, num / den);
        }
        check(worst < 1e-9, "sparse LDL^T matches dense LU (max relative diff)", worst);

        // The 1013-node network's matrix: sparse vs dense, and how little fill-in the ordering gives
        const char* paths[] = {"circuits/grid1000.net", "../circuits/grid1000.net"};
        for (const char* p : paths) {
            std::ifstream probe(p);
            if (!probe) continue;
            std::ostringstream quiet;
            auto* old = std::cout.rdbuf(quiet.rdbuf());
            Network net(loadNetlist(p).elements);
            net.updateSwitches(0.0); net.formG(); net.Gdivide(); net.LU();
            std::cout.rdbuf(old);
            check(net.nnzFactor() < 4 * net.nnzG_UU(), "grid1000: fill-in of L stays small (non-zeros in L)", double(net.nnzFactor()));
            break;
        }

        // Singular matrix (a floating pair of nodes) is reported
        SparseMatrix S(2, 2);
        S.add(0, 0, 1.0); S.add(1, 1, 1.0); S.add(0, 1, -1.0); S.add(1, 0, -1.0); S.finalize();
        bool threw = false;
        try { SparseLDL fac; fac.factor(S); } catch (const std::runtime_error&) { threw = true; }
        check(threw, "sparse LDL^T reports a singular matrix", 0.0);
    }

    std::printf("\n%s\n", failures ? "SOME TESTS FAILED" : "All tests passed");
    return failures ? 1 : 0;
}
