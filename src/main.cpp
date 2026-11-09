#include <chrono>
#include <exception>
#include <iostream>

#include "circuit.h"
#include "config.h"
#include "netlist.h"
#include "network.h"

// Form the conductance matrix, divide it into GUU, GUK, GKU, GKK and
// LU-decompose GUU. Only needed at the start and when a switch/fault changes
// state, because G is constant otherwise.
static void refactor(Network& EMTDC) {
    EMTDC.formG();
    EMTDC.Gdivide();
    EMTDC.LU();
}

// Usage:  emtsim                         -> circuit in circuit.cpp
//         emtsim circuits/grid100.net    -> circuit from a netlist file
int main(int argc, char** argv) {
    try {
        std::cout << "\nStart running the simulation..... \n\n";
        double Time = config::srtTime;

        const auto wallStart = std::chrono::steady_clock::now();

        // Initialize the network with the circuit from a netlist file or circuit.cpp
        Network EMTDC(argc > 1 ? loadNetlist(argv[1]).elements : buildCircuit().elements);
        // Set the initial switch/fault states and factorise G
        EMTDC.updateSwitches(Time);
        refactor(EMTDC);

        // Enter the main time loop
        while (Time < config::stpTime) {
            // Divide node voltage vector into VU and VK
            EMTDC.Vdivide();
            // Advance one time step
            Time = Time + config::Dt;
            // Apply/clear faults; rebuild G if anything switched
            if (EMTDC.updateSwitches(Time)) refactor(EMTDC);
            // Update the voltage sources
            EMTDC.Vkupdate(Time);
            // Calculate the history current of each branch
            EMTDC.calcBrnHistory();
            // Calculate the current injection at each node
            EMTDC.calcinjection();
            // Divide IH in to IU and IK
            EMTDC.Idivide();
            // Calculate new IU
            EMTDC.calIUnew();
            // Calculate new VU
            EMTDC.calcnewV();
            // Reconstruct node voltage vector
            EMTDC.reconstruct_V();
            // Calculate branch currents of each element
            EMTDC.calcNewbranchI();
            // Record the voltages of all nodes
            EMTDC.recordv(Time);
        }

        // Setup + time loop, excluding writing the results
        const double simMs =
            std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - wallStart).count();

        EMTDC.writeResults("results.csv");
        EMTDC.writeEvents("events.csv");
        std::cout << "\nDone: " << EMTDC.time().size() << " steps, " << EMTDC.numNodes() << " nodes.\n"
                  << "Sparse LDL: G_UU has " << EMTDC.nnzG_UU() << " non-zeros, L has " << EMTDC.nnzFactor()
                  << " below the diagonal, " << EMTDC.factorisations() << " factorisations\n"
                  << "Simulation time: " << simMs << " ms\n"
                  << "Results written to results.csv and events.csv. Plot them with:  python plot.py\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }
}
