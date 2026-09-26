// SPDX-License-Identifier: GPL-3.0-or-later
// CeylonDemon - Copyright (C) 2026 Madushan Dissanayake. GNU GPL v3+; see LICENSE.
#pragma once

#include <algorithm>
#include <iostream>
#include <string>

// Search defaults live here so release and tuning builds share one source of
// truth. A normal build keeps them as compile-time constants. A build made with
// -DCEYLON_TUNE exposes the same values as UCI spin options for SPSA.
namespace tuning {

#ifdef CEYLON_TUNE
#define CEYLON_PARAM(name, uci, value, lo, hi) inline int name = value;
#else
#define CEYLON_PARAM(name, uci, value, lo, hi) inline constexpr int name = value;
#endif

CEYLON_PARAM(RazorMargin,       "Razor Margin",        200,   0, 1000)
CEYLON_PARAM(RazorSlope,        "Razor Slope",         150,   0, 1000)
CEYLON_PARAM(RfpMargin,         "RFP Margin",           80,   0,  500)
CEYLON_PARAM(RfpSlope,          "RFP Improving",        60,   0,  500)
CEYLON_PARAM(NullBase,          "NMP Base",              3,   1,    8)
CEYLON_PARAM(NullDepthDiv,      "NMP Depth Divisor",     3,   1,   12)
CEYLON_PARAM(NullEvalDiv,       "NMP Eval Divisor",    200,  25, 1000)
CEYLON_PARAM(NullEvalCap,       "NMP Eval Cap",          3,   0,    8)
CEYLON_PARAM(ProbCutMargin,     "ProbCut Margin",      180,   0, 1000)
CEYLON_PARAM(LmrBase100,        "LMR Base x100",        75,   0,  300)
CEYLON_PARAM(LmrDivisor100,     "LMR Divisor x100",    225,  50,  800)
CEYLON_PARAM(HistoryBonus,      "History Bonus",        16,   1,  128)
CEYLON_PARAM(HistoryMax,        "History Max",        2000, 100, 8000)
CEYLON_PARAM(AspirationDelta,   "Aspiration Delta",     25,   5,  500)
CEYLON_PARAM(TimeBase100,       "Time Base x100",      135,  50,  300)
CEYLON_PARAM(TimeStability100,  "Time Stability x100",   7,   0,   50)
CEYLON_PARAM(TimePressureMin100,"Time Pressure Min x100",-15,-100,   0)
CEYLON_PARAM(TimePressureMax100,"Time Pressure Max x100", 50,   0,  200)

#undef CEYLON_PARAM

#ifdef CEYLON_TUNE
struct Param {
    const char* uci;
    int* value;
    int defaultValue;
    int minValue;
    int maxValue;
};

inline Param params[] = {
#define CEYLON_ENTRY(name, uci, value, lo, hi) {uci, &name, value, lo, hi},
    CEYLON_ENTRY(RazorMargin,       "Razor Margin",        200,   0, 1000)
    CEYLON_ENTRY(RazorSlope,        "Razor Slope",         150,   0, 1000)
    CEYLON_ENTRY(RfpMargin,         "RFP Margin",           80,   0,  500)
    CEYLON_ENTRY(RfpSlope,          "RFP Improving",        60,   0,  500)
    CEYLON_ENTRY(NullBase,          "NMP Base",              3,   1,    8)
    CEYLON_ENTRY(NullDepthDiv,      "NMP Depth Divisor",     3,   1,   12)
    CEYLON_ENTRY(NullEvalDiv,       "NMP Eval Divisor",    200,  25, 1000)
    CEYLON_ENTRY(NullEvalCap,       "NMP Eval Cap",          3,   0,    8)
    CEYLON_ENTRY(ProbCutMargin,     "ProbCut Margin",      180,   0, 1000)
    CEYLON_ENTRY(LmrBase100,        "LMR Base x100",        75,   0,  300)
    CEYLON_ENTRY(LmrDivisor100,     "LMR Divisor x100",    225,  50,  800)
    CEYLON_ENTRY(HistoryBonus,      "History Bonus",        16,   1,  128)
    CEYLON_ENTRY(HistoryMax,        "History Max",        2000, 100, 8000)
    CEYLON_ENTRY(AspirationDelta,   "Aspiration Delta",     25,   5,  500)
    CEYLON_ENTRY(TimeBase100,       "Time Base x100",      135,  50,  300)
    CEYLON_ENTRY(TimeStability100,  "Time Stability x100",   7,   0,   50)
    CEYLON_ENTRY(TimePressureMin100,"Time Pressure Min x100",-15,-100,   0)
    CEYLON_ENTRY(TimePressureMax100,"Time Pressure Max x100", 50,   0,  200)
#undef CEYLON_ENTRY
};

inline void printUciOptions() {
    for (const Param& p : params)
        std::cout << "option name " << p.uci << " type spin default "
                  << p.defaultValue << " min " << p.minValue
                  << " max " << p.maxValue << "\n";
}

inline bool setOption(const std::string& name, const std::string& value) {
    for (Param& p : params) {
        if (name != p.uci) continue;
        try {
            *p.value = std::clamp(std::stoi(value), p.minValue, p.maxValue);
            return true;
        } catch (...) {
            return false;
        }
    }
    return false;
}
#else
inline void printUciOptions() {}
inline bool setOption(const std::string&, const std::string&) { return false; }
#endif

} // namespace tuning
