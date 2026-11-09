#include "netlist.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>

Circuit loadNetlist(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("Cannot open netlist " + path);

    Circuit ckt;
    std::string raw;
    for (int lineno = 1; std::getline(in, raw); ++lineno) {
        const std::string where = path + ":" + std::to_string(lineno);
        std::istringstream ss(raw.substr(0, raw.find('#')));
        std::vector<std::string> tok;
        for (std::string t; ss >> t;) tok.push_back(t);
        if (tok.empty()) continue;
        if (tok.size() < 4) throw std::runtime_error(where + ": expected 'TYPE from to ...'");

        std::string kind = tok[0];
        std::transform(kind.begin(), kind.end(), kind.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
        try {
            const int a = std::stoi(tok[1]), b = std::stoi(tok[2]);
            auto num = [&](std::size_t i) { return std::stod(tok.at(i)); };

            if (kind == "R") ckt.addR(a, b, num(3));
            else if (kind == "L") ckt.addL(a, b, num(3));
            else if (kind == "C") ckt.addC(a, b, num(3));
            else if (kind == "V") {
                if (tok.size() < 6) throw std::runtime_error("V needs magnitude angle frequency [resistance]");
                ckt.addSource(a, b, num(3), num(4), num(5), tok.size() > 6 ? num(6) : 0.0);
            } else if (kind == "SW") {
                if (tok[3] != "closed" && tok[3] != "open")
                    throw std::runtime_error("SW state must be 'closed' or 'open'");
                std::vector<double> times;
                for (std::size_t i = 4; i < tok.size(); ++i) times.push_back(num(i));
                ckt.addSwitch(a, b, tok[3] == "closed", times);
            } else if (kind == "F") {
                ckt.addFault(a, b, num(3), tok.size() > 4 ? std::optional<double>(num(4)) : std::nullopt);
            } else {
                throw std::runtime_error("unknown element type '" + tok[0] + "'");
            }
        } catch (const std::logic_error&) {   // stoi/stod/at failures
            throw std::runtime_error(where + ": invalid number or missing value");
        } catch (const std::runtime_error& e) {
            throw std::runtime_error(where + ": " + e.what());
        }
    }
    return ckt;
}
