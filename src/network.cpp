#include "network.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

Network::Network(std::vector<std::unique_ptr<Element>> elements) : comp_list_(std::move(elements)) {
    for (const auto& e : comp_list_) numNodes_ = std::max({numNodes_, e->strnode, e->stpnode});

    // Known nodes are the nodes whose voltage is fixed by an ideal voltage source.
    // Sources with a series resistance are modelled as Norton equivalents and
    // do not create known nodes.
    std::vector<int> known;
    for (const auto& e : comp_list_) {
        if (!e->isIdealSource()) continue;
        if (e->strnode != 0 && e->stpnode != 0)
            throw std::invalid_argument("An ideal voltage source must have one node connected to ground (node 0)");
        const int node = (e->strnode == 0) ? e->stpnode : e->strnode;
        if (std::find(known.begin(), known.end(), node) != known.end())
            throw std::invalid_argument("Two ideal voltage sources on node " + std::to_string(node));
        known.push_back(node);
    }

    // Internal node ordering: unknown nodes first, then known nodes, so that G can
    // be partitioned into G_UU, G_UK, G_KU, G_KK regardless of how the user
    // numbered the nodes.
    for (int n = 1; n <= numNodes_; ++n)
        if (std::find(known.begin(), known.end(), n) == known.end()) order_.push_back(n);
    order_.insert(order_.end(), known.begin(), known.end());

    idx_.assign(numNodes_ + 1, -1);
    for (int i = 0; i < numNodes_; ++i) idx_[order_[i]] = i;
    for (auto& e : comp_list_) {
        e->fi = idx_[e->strnode];
        e->ti = idx_[e->stpnode];
    }

    src_count_ = static_cast<int>(known.size());
    unknownNodes_ = numNodes_ - src_count_;
    Vn_.assign(numNodes_, 0.0);
    I_H_.assign(numNodes_, 0.0);

    std::cout << "No of ideal sources " << src_count_ << "\n";
    std::cout << "No of unknown nodes " << unknownNodes_ << "\n\n";
}

bool Network::updateSwitches(double t) {
    bool changed = false;
    for (auto& e : comp_list_)
        if (auto* sw = dynamic_cast<Switch*>(e.get())) changed = sw->update(t) || changed;
    return changed;
}

void Network::formG() {
    G_ = SparseMatrix(numNodes_, numNodes_);
    for (const auto& e : comp_list_) {
        if (e->strnode == 0 && e->stpnode == 0) std::cout << "*** Both nodes of a element is zero ***\n";
        if (e->strnode == e->stpnode) std::cout << "*** Both nodes of a element is same ***\n";

        // Ideal sources have no conductance; their node voltage is imposed via V_K
        if (e->isIdealSource()) continue;

        const double g = 1.0 / e->Reff;
        const int f = e->fi, t = e->ti;
        if (f >= 0) G_.add(f, f, g);
        if (t >= 0) G_.add(t, t, g);
        if (f >= 0 && t >= 0) {
            G_.add(f, t, -g);
            G_.add(t, f, -g);
        }
    }
    G_.finalize();   // entries stamped at the same position are summed
    if (numNodes_ <= 10) G_.toDense().print(std::cout, "G");   // printing a large G would swamp the output
}

void Network::Gdivide() {
    const std::size_t u = unknownNodes_, k = src_count_;
    G_UU_ = G_.block(0, 0, u, u);
    G_UK_ = G_.block(0, u, u, k);
    G_KU_ = G_.block(u, 0, k, u);
    G_KK_ = G_.block(u, u, k, k);
}

void Network::LU() {
    G_UU_LU_.factor(G_UU_);
    ++factorisations_;
}

void Network::Vdivide() {
    V_U_.assign(Vn_.begin(), Vn_.begin() + unknownNodes_);
    V_K_.assign(Vn_.begin() + unknownNodes_, Vn_.end());
}

void Network::Vkupdate(double t) {
    for (auto& e : comp_list_) {
        auto* src = dynamic_cast<Source*>(e.get());
        if (!src) continue;
        src->update(t);
        if (src->ideal) {
            // Source raises the potential from strnode to stpnode by E
            if (src->strnode == 0) V_K_[src->ti - unknownNodes_] = src->E;
            else                   V_K_[src->fi - unknownNodes_] = -src->E;
        }
    }
}

void Network::calcBrnHistory() {
    for (auto& e : comp_list_) e->calcHistory();
}

void Network::calcinjection() {
    I_H_.assign(numNodes_, 0.0);
    for (const auto& e : comp_list_) {
        if (e->isIdealSource()) continue;
        // Branch current i = V/Reff + ihistory flows from strnode to stpnode,
        // so ihistory leaves strnode and enters stpnode. Ground (-1) is skipped.
        if (e->fi >= 0) I_H_[e->fi] -= e->ihistory;
        if (e->ti >= 0) I_H_[e->ti] += e->ihistory;
    }
}

void Network::Idivide() {
    I_U_.assign(I_H_.begin(), I_H_.begin() + unknownNodes_);
    I_K_.assign(I_H_.begin() + unknownNodes_, I_H_.end());
}

void Network::calIUnew() {
    I_U_new_ = I_U_;
    if (src_count_ > 0) {
        const Vector gv = G_UK_ * V_K_;
        for (int i = 0; i < unknownNodes_; ++i) I_U_new_[i] -= gv[i];
    }
}

void Network::calcnewV() { V_U_ = G_UU_LU_.solve(I_U_new_); }

void Network::reconstruct_V() {
    std::copy(V_U_.begin(), V_U_.end(), Vn_.begin());
    std::copy(V_K_.begin(), V_K_.end(), Vn_.begin() + unknownNodes_);
}

void Network::calcNewbranchI() {
    for (auto& e : comp_list_) {
        // Branch voltage V = V(strnode) - V(stpnode), ground = 0
        const double Vfrom = e->fi >= 0 ? Vn_[e->fi] : 0.0;
        const double Vto = e->ti >= 0 ? Vn_[e->ti] : 0.0;
        const double V = Vfrom - Vto;
        e->Vlast = V;
        if (e->isIdealSource()) continue;
        // R: ihistory = 0, Norton source: ihistory = E/R, L/C: trapezoidal history
        e->Ilast = V / e->Reff + e->ihistory;
    }
}

double Network::nodeVoltage(int node) const { return node == 0 ? 0.0 : Vn_[idx_.at(node)]; }

void Network::recordv(double t) {
    time_.push_back(t);
    Vector row(numNodes_);
    for (int n = 1; n <= numNodes_; ++n) row[n - 1] = nodeVoltage(n);
    trace_.push_back(std::move(row));
}

void Network::writeResults(const std::string& path) const {
    std::ofstream f(path);
    if (!f) throw std::runtime_error("Cannot write " + path);
    f << "time";
    for (int n = 1; n <= numNodes_; ++n) f << ",V" << n;
    f << "\n" << std::setprecision(17);   // full double precision, so results compare exactly
    for (std::size_t s = 0; s < time_.size(); ++s) {
        f << time_[s];
        for (double v : trace_[s]) f << "," << v;
        f << "\n";
    }
}

void Network::writeEvents(const std::string& path) const {
    std::ofstream f(path);
    if (!f) throw std::runtime_error("Cannot write " + path);
    f << "type,t_start,t_end\n" << std::setprecision(12);
    for (const auto& e : comp_list_) {
        if (auto* fl = dynamic_cast<const Fault*>(e.get())) {
            f << "fault," << fl->t_apply << ",";
            if (fl->t_clear) f << *fl->t_clear;
            f << "\n";
        } else if (auto* sw = dynamic_cast<const Switch*>(e.get())) {
            for (double ts : sw->t_switch) f << "switch," << ts << ",\n";
        }
    }
}
