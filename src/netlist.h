#pragma once

// Read an EMTSim netlist file. One element per line (node 0 = ground; '#' starts a comment):
//
//   R  from to  ohm
//   L  from to  henry
//   C  from to  farad
//   V  from to  magnitude(peak V) angle(deg) frequency(Hz) [resistance]   no resistance = ideal source
//   SW from to  closed|open  t1 [t2 ...]     toggles state at each time (s)
//   F  from to  t_apply [t_clear]
//
// The same format is read by the Python version (netlist.py).

#include <string>

#include "circuit.h"

Circuit loadNetlist(const std::string& path);  // throws std::runtime_error with file:line on errors
