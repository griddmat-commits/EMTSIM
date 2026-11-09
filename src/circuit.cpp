#include "circuit.h"

// Two-source, 8-node network with a breaker and a fault
//
//  Source A                                                         Source B
//  (Norton)                                                          (ideal)
//    (1)--R--(2)--L--(3)--SW1--(4)--R--(5)--L--(6)--L--(7)--R--(8)
//     |               |                 |       |                  |
//    Vs1+Rs1      Rld1 || C1           C2   Rld2 || F1            Vs2
//     |               |                 |       |                  |
//    ---             ---               ---     ---                ---
//
// Source A : 230 V rms, 50 Hz, 0 deg, 0.1 ohm internal resistance (Norton model)
// Source B : 230 V rms, 50 Hz, -15 deg, ideal (node 8 is a known node)
// Bus 3    : load A = 25 ohm || 30 uF
// Node 5   : 20 uF line-charging capacitance
// Bus 6    : load B = 30 ohm
//
// Events
//   20 ms : bolted fault (0.01 ohm) bus 6 to ground
//   40 ms : fault cleared
//   55 ms : breaker SW1 opens  -> the two halves of the network separate
//   80 ms : breaker SW1 recloses

Circuit buildCircuit() {
    Circuit ckt;

    // Sources:  from, to, peak voltage, angle (deg), frequency (Hz), [resistance]
    ckt.addSource(0, 1, 325.0, 0.0, 50.0, 0.1);   // VS1, Norton
    ckt.addSource(0, 8, 325.0, -15.0, 50.0);      // VS2, ideal

    // Source A side
    ckt.addR(1, 2, 0.2);
    ckt.addL(2, 3, 3.0e-3);
    ckt.addR(3, 0, 25.0);       // load A
    ckt.addC(3, 0, 30.0e-6);

    // Breaker between the two halves: closed, opens at 55 ms, recloses at 80 ms
    ckt.addSwitch(3, 4, true, {0.055, 0.080});

    // Line from bus 4 to bus 6
    ckt.addR(4, 5, 0.3);
    ckt.addL(5, 6, 4.0e-3);
    ckt.addC(5, 0, 20.0e-6);    // line charging

    // Bus 6 load and fault
    ckt.addR(6, 0, 30.0);       // load B
    ckt.addFault(6, 0, 0.020, 0.040);

    // Source B side
    ckt.addL(6, 7, 2.0e-3);
    ckt.addR(7, 8, 0.2);

    return ckt;
}

// ---------------------------------------------------------------------------
// Previous example: radial feeder with a fault at the load bus (3 nodes)
//
//    ckt.addSource(0, 1, 325.0, 0.0, 50.0, 0.1);
//    ckt.addR(1, 2, 0.5);
//    ckt.addL(2, 3, 5.0e-3);
//    ckt.addR(3, 0, 20.0);
//    ckt.addC(3, 0, 50.0e-6);
//    ckt.addFault(3, 0, 0.040, 0.070);
