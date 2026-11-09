#pragma once

// Simulation settings
namespace config {

constexpr double Dt = 100e-6;                    // time step (s)
constexpr double srtTime = 0.0;                  // start time (s)
constexpr double stpTime = srtTime + 1000 * Dt;  // stop time (s)

}  // namespace config
