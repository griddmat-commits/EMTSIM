#pragma once

// The network solver: Dommel's nodal method.
//
// Every time step the network is a resistive circuit  G v = i_h.  Nodes are
// split into unknown (U) and known (K, fixed by ideal sources) nodes:
//
//     G_UU v_U = i_U - G_UK v_K
//
// G is stored as a sparse matrix. G_UU is symmetric positive definite, so it is
// factorised with a sparse LDL^T (minimum-degree ordering, no pivoting), once at
// the start and again only when a switch operates.

#include <memory>
#include <string>
#include <vector>

#include "elements.h"
#include "linalg.h"

class Network {
public:
    explicit Network(std::vector<std::unique_ptr<Element>> elements);

    bool updateSwitches(double t);  // apply/clear faults and switches; true if any changed

    void formG();            // assemble the conductance matrix
    void Gdivide();          // split G into G_UU, G_UK, G_KU, G_KK
    void LU();               // factorise G_UU (sparse LDL^T)
    void Vdivide();          // split node voltages into V_U, V_K
    void Vkupdate(double t); // update sources and known node voltages V_K
    void calcBrnHistory();   // history current of every branch
    void calcinjection();    // nodal current injections I_H
    void Idivide();          // split I_H into I_U, I_K
    void calIUnew();         // I_U_new = I_U - G_UK * V_K
    void calcnewV();         // solve G_UU V_U = I_U_new
    void reconstruct_V();    // Vn = [V_U ; V_K]
    void calcNewbranchI();   // branch voltages and currents for the next history term
    void recordv(double t);  // record all node voltages

    void writeResults(const std::string& path) const;  // time, V1..Vn as CSV
    void writeEvents(const std::string& path) const;   // fault/switch times as CSV

    int numNodes() const { return numNodes_; }
    double nodeVoltage(int node) const;                // user node number, 0 = ground
    const std::vector<double>& time() const { return time_; }
    const std::vector<Vector>& trace() const { return trace_; }
    std::size_t nnzG_UU() const { return G_UU_.nnz(); }                 // non-zeros in G_UU
    std::size_t nnzFactor() const { return G_UU_LU_.nnzL(); }           // non-zeros in L (below the diagonal)
    int factorisations() const { return factorisations_; }

private:
    std::vector<std::unique_ptr<Element>> comp_list_;
    int numNodes_ = 0, unknownNodes_ = 0, src_count_ = 0;
    std::vector<int> order_;  // internal index -> user node
    std::vector<int> idx_;    // user node -> internal index (ground -> -1)

    SparseMatrix G_, G_UU_, G_UK_, G_KU_, G_KK_;
    SparseLDL G_UU_LU_;
    int factorisations_ = 0;
    Vector Vn_, V_U_, V_K_, I_H_, I_U_, I_K_, I_U_new_;

    std::vector<double> time_;
    std::vector<Vector> trace_;  // one row per time step: voltages of nodes 1..numNodes
};
