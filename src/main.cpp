// SPDX-License-Identifier: GPL-3.0-or-later
//
// CeylonDemon — a UCI chess engine
// Copyright (C) 2026 Madushan Dissanayake
// Built on the NARC Engine codebase by the same author (GPLv3).
//
// Provenance: CeylonDemon 1.7 and later are an independent reimplementation.
// Versions up to and including 1.6 were built on a Stockfish-derived
// foundation and are superseded. See NOTICE.md for the full statement.
//
// This program is free software: you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version. This program is distributed WITHOUT ANY WARRANTY; without even the
// implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License (LICENSE file) for more details.
//
// Build: see build.ps1
#include "search.h"
#include <fstream>
#include <iomanip>
#include <random>
#include <thread>

static constexpr const char* BENCH_FENS[] = {
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
    "rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2",
    "rnbqkb1r/pppp1ppp/5n2/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3",
    "r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3",
    "rnbqk2r/pppp1ppp/5n2/2b1p3/2B1P3/5N2/PPPP1PPP/RNBQ1RK1 b kq - 5 4",
    "r1bq1rk1/pppp1ppp/2n2n2/2b1p3/2B1P3/2NP1N2/PPP2PPP/R1BQ1RK1 w - - 4 7",
    "rnbqkbnr/pp2pppp/2pp4/8/3PP3/8/PPP2PPP/RNBQKBNR b KQkq - 0 3",
    "rnbqk2r/pp2bppp/2pp1n2/4p3/3PP3/2N2N2/PPP1BPPP/R1BQK2R w KQkq - 2 7",
    "r1bqk2r/pp2bppp/2np1n2/4p3/3PP3/2N2N2/PPPQBPPP/R3K2R w KQkq - 4 8",
    "rnbqkbnr/ppp1pppp/8/3p4/2PP4/8/PP2PPPP/RNBQKBNR b KQkq - 0 2",
    "rnbqkb1r/pp2pppp/2p2n2/3p4/2PP4/2N1P3/PP3PPP/R1BQKBNR b KQkq - 0 4",
    "r1bqk2r/pp2bppp/2n1pn2/2pp4/2PP4/2N1PN2/PPQ1BPPP/R3K2R w KQkq - 2 8",
    "rnbqkbnr/pppppp1p/6p1/8/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2",
    "rnbqk2r/ppppppbp/5np1/8/4P3/2N2N2/PPPP1PPP/R1BQKB1R w KQkq - 4 4",
    "r1bq1rk1/ppp1ppbp/2np1np1/8/2BPP3/2N2N2/PP3PPP/R1BQ1RK1 w - - 2 8",
    "rnbqkbnr/pppppppp/8/8/2P5/8/PP1PPPPP/RNBQKBNR b KQkq - 0 1",
    "rnbqkbnr/pp2pppp/8/2pp4/2P5/5N2/PP1PPPPP/RNBQKB1R w KQkq - 0 3",
    "r1bqk2r/pp2bppp/2n1pn2/2pp4/2P5/1PN1PN2/PB1P1PPP/R2QKB1R w KQkq - 2 7",
    "rnbqkbnr/pppppppp/8/8/3P4/8/PPP1PPPP/RNBQKBNR b KQkq - 0 1",
    "rnbqkb1r/pppppppp/5n2/8/3P4/2N5/PPP1PPPP/R1BQKBNR b KQkq - 2 2",
    "r1bqk2r/pppp1ppp/2n2n2/2b1p3/3P4/2N1PN2/PPP1BPPP/R1BQK2R w KQkq - 4 6",
    "r3k2r/ppp2ppp/2npbn2/4p3/2B1P3/2NP1N2/PPP2PPP/R1B2RK1 w kq - 2 9",
    "2rq1rk1/pp2bppp/2npbn2/4p3/2B1P3/2NP1N2/PPPQ1PPP/R1B2RK1 w - - 6 11",
    "8/5pk1/6p1/8/4P3/5K2/8/8 w - - 0 40",
    "8/8/3k4/3p4/3P4/3K4/8/8 w - - 0 50",
    "8/2p2pk1/3p2p1/1p6/1P2P3/2P2P2/3K2P1/8 w - - 0 35",
    "8/5pk1/4p1p1/3pP3/3P1P2/4K3/8/8 w - - 0 45",
    "4r1k1/1p3ppp/p1p5/8/3P4/P3P3/1P3PPP/4R1K1 w - - 0 24",
    "2r3k1/5ppp/1p2p3/p2p4/P2P4/1P2P1P1/5P1P/2R3K1 w - - 0 28",
    "4rrk1/1pp2ppp/p1np4/8/2P1P3/2N2P2/PP3P1P/3RR1K1 w - - 0 20",
    "2r2rk1/pp1b1ppp/2n1p3/3pP3/3P4/2P2N2/PP3PPP/R1B1R1K1 w - - 0 18",
    "3q1rk1/1p3ppp/p1nr4/3p4/3P4/2N1P3/PPQ2PPP/2RR2K1 w - - 0 22"
};

static Position rootPos;
static std::thread searchThread;
static std::atomic<bool> searchFinished{true};
static int moveOverhead = 30; // ms

static void stopSearchThread() {
    Stopped = true;
    if (searchThread.joinable()) searchThread.join();
    searchFinished = true;
}

static Move parseUciMove(const Position& pos, const std::string& str) {
    MoveList ml;
    genMoves<false>(pos, ml);
    for (int i = 0; i < ml.count; i++)
        if (moveStr(ml.list[i].move) == str) return ml.list[i].move;
    return MOVE_NONE;
}

static void setPosition(std::istringstream& is) {
    std::string token;
    is >> token;
    std::string fen;
    if (token == "startpos") {
        fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
        is >> token; // consume "moves" if present
    } else if (token == "fen") {
        while (is >> token && token != "moves") fen += token + " ";
    }
    rootPos.setFen(fen);
    gameHistLen = 0;
    gameHist[gameHistLen++] = rootPos.key;

    Undo u;
    while (is >> token) {
        Move m = parseUciMove(rootPos, token);
        if (m == MOVE_NONE) break;
        if (!rootPos.make(m, u)) break; // illegal move in the list: stop here
        rootPos.commitAccumulator();
        gameHist[gameHistLen++] = rootPos.key;
        if (gameHistLen >= 1000) { // keep buffer safe; old entries no longer needed
            memmove(gameHist, gameHist + 500, 500 * sizeof(u64));
            gameHistLen -= 500;
        }
    }
}

static void startSearch(const SearchLimits& limits) {
    stopSearchThread();
    Stopped = false;
    searchFinished = false;
    searchLimits = limits;
    searchStart = std::chrono::steady_clock::now();
    searchThread = std::thread([] {
        Move best = searchPosition(rootPos); // Lazy SMP across `Threads` threads
        // in infinite mode, UCI requires waiting for "stop" before bestmove
        while (searchLimits.infinite && !Stopped)
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        // Publish completion before bestmove. A GUI sends isready only after
        // receiving that line; the UCI thread can then join this worker before
        // answering, eliminating the old post-bestmove/pre-readyok stall.
        searchFinished = true;
        std::cout << "bestmove " << moveStr(best) << std::endl;
    });
}

static void goCommand(std::istringstream& is) {
    SearchLimits limits;
    s64 wtime = -1, btime = -1, winc = 0, binc = 0, movetime = -1;
    int movestogo = 0;
    std::string token;

    while (is >> token) {
        if (token == "wtime") is >> wtime;
        else if (token == "btime") is >> btime;
        else if (token == "winc") is >> winc;
        else if (token == "binc") is >> binc;
        else if (token == "movestogo") is >> movestogo;
        else if (token == "movetime") is >> movetime;
        else if (token == "depth") is >> limits.depthLimit;
        else if (token == "nodes") is >> limits.nodeLimit;
        else if (token == "infinite") limits.infinite = true;
        else if (token == "perft") {
            int d; is >> d;
            auto t0 = std::chrono::steady_clock::now();
            u64 n = perft(rootPos, d);
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - t0).count();
            std::cout << "perft " << d << ": " << n << " nodes, " << ms << " ms, "
                      << (ms > 0 ? n * 1000 / ms : 0) << " nps" << std::endl;
            return;
        }
    }

    s64 myTime = rootPos.stm == WHITE ? wtime : btime;
    s64 myInc  = rootPos.stm == WHITE ? winc : binc;

    if (movetime >= 0) {
        // UCI "movetime" is an explicit analysis budget, not a game clock.
        // Move Overhead applies only to wtime/btime where flag protection is
        // needed. Subtracting it here cost 30% of a 100 ms test budget.
        limits.softLimit = limits.hardLimit = std::max<s64>(1, movetime);
    } else if (myTime >= 0) {
        myTime = std::max<s64>(1, myTime - moveOverhead);
        // Without an explicit movestogo, retain a deeper reserve in the
        // opening and spend it progressively as the game advances.
        int estimated = std::clamp(50 - rootPos.fullmove / 2, 20, 40);
        int mtg = movestogo > 0 ? std::clamp(movestogo, 2, 40) : estimated;
        bool conversionTime = popcount(rootPos.occupied()) <= 10
                           && std::abs(evaluate(rootPos)) >= 400;
        // Increment is replenished after every completed move. Discounting it
        // to 75% left several seconds unused in ordinary 10s+100ms games and
        // effectively gave the opponent persistent time odds.
        s64 usableInc = conversionTime ? myInc * 7 / 8 : myInc;
        s64 base = myTime / mtg + usableInc;
        // The previous 50%-of-clock hard cap could spend a full second from a
        // two-second clock whenever one iteration ran long. Keep the normal
        // budget slightly less conservative, but cap exceptional moves at 20%
        // of the remaining clock so increment games cannot spiral into flags.
        limits.softLimit = std::max<s64>(1, base);
        // In increment games, the old 20%-of-clock cap became far too
        // conservative once the remaining clock approached the increment.
        // Normally spend up to 75% of the increment and retain 20% of the
        // clock. In an already decisive sparse ending, use up to 87.5% of the
        // increment while keeping a smaller 10% flag-safety reserve.
        s64 clockBudget = conversionTime ? myTime * 9 / 10 : myTime * 4 / 5;
        s64 reserveCap = conversionTime ? myTime / 10 : myTime / 5;
        s64 incrementBudget = std::min(clockBudget, usableInc);
        s64 safeHardCap = std::max(reserveCap, incrementBudget);
        limits.hardLimit = std::max<s64>(1, std::min(base * 3, safeHardCap));
        limits.softLimit = std::min(limits.softLimit, limits.hardLimit);
    }
    startSearch(limits);
}

static void benchCommand(std::istringstream& is) {
    int depth = 10;
    int hashMb = 16;
    is >> depth >> hashMb;
    depth = std::clamp(depth, 1, 64);
    hashMb = std::clamp(hashMb, 1, 4096);

    stopSearchThread();
    const bool previousSilent = SilentSearch;
    SilentSearch = true;
    tt.resize(hashMb);

    u64 total = 0;
    u64 signature = 1469598103934665603ULL;
    auto begin = std::chrono::steady_clock::now();

    for (const char* fen : BENCH_FENS) {
        rootPos.setFen(fen);
        gameHistLen = 0;
        gameHist[gameHistLen++] = rootPos.key;
        tt.clear();
        clearAllHistories();
        Stopped = false;
        searchLimits = SearchLimits{};
        searchLimits.depthLimit = depth;
        searchStart = std::chrono::steady_clock::now();
        searchSync(rootPos);
        u64 nodes = threadData[0]->nodes;
        total += nodes;
        signature ^= nodes;
        signature *= 1099511628211ULL;
    }

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - begin).count();
    SilentSearch = previousSilent;
    rootPos.setFen(BENCH_FENS[0]);
    gameHistLen = 0;
    gameHist[gameHistLen++] = rootPos.key;
    std::cout << "bench depth " << depth
              << " positions " << std::size(BENCH_FENS)
              << " nodes " << total
              << " signature " << std::hex << signature << std::dec
              << " time " << ms
              << " nps " << (ms > 0 ? total * 1000 / u64(ms) : 0)
              << std::endl;
}

static void featureHashCommand() {
    u64 signature = 1469598103934665603ULL;
    for (int head = 0; head < resonance::HEAD_NB; head++)
        for (int perspective = 0; perspective < 2; perspective++)
            for (int king = 0; king < 64; king++)
                for (int colour = 0; colour < 2; colour++)
                    for (int piece = 0; piece < 6; piece++)
                        for (int square = 0; square < 64; square++) {
                            signature ^= u64(resonance::featureIndex(
                                head, perspective, king, colour, piece, square));
                            signature *= 1099511628211ULL;
                        }
    std::cout << "featurehash " << std::hex << signature << std::dec << std::endl;
}

// ----------------------------- Training data generation ----------------------
// datagen <games> <outfile> [nodes_per_move] [seed]
// datagenbin <games> <outfile> [nodes_per_move] [seed]
//
// The binary form is the trainer's compact 32-byte record:
//   0..7   occupancy u64, little endian
//   8..23  occupied piece codes, two nibbles per byte, square order
//   24..25 White score int16, little endian
//   26     game result: 0 black win, 1 draw, 2 white win
//   27     side to move: 0 white, 1 black
//   28     sample ordinal within game (resets to zero at each game boundary)
//   29..31 reserved, zero
// Separate engine processes are used for parallel generation so every worker
// owns its TT, histories, search state, and output shard.
static std::array<u8, 32> trainingRecord(const Position& pos, int whiteScore,
                                         int sampleOrdinal) {
    std::array<u8, 32> record{};
    const u64 occupancy = pos.occupied();
    for (int i = 0; i < 8; i++) record[i] = u8(occupancy >> (i * 8));

    int pieceIndex = 0;
    for (int sq = 0; sq < 64; sq++) {
        if (!(occupancy & bit(sq))) continue;
        const int code = pos.colorOn(sq) * 8 + pos.board[sq];
        const int byteIndex = 8 + pieceIndex / 2;
        if (pieceIndex & 1) record[byteIndex] |= u8(code << 4);
        else                record[byteIndex]  = u8(code);
        pieceIndex++;
    }

    const int clampedScore = std::clamp(whiteScore, -32768, 32767);
    const u16 scoreBits = u16(int16_t(clampedScore));
    record[24] = u8(scoreBits);
    record[25] = u8(scoreBits >> 8);
    record[27] = u8(pos.stm);
    record[28] = u8(std::clamp(sampleOrdinal, 0, 255));
    return record;
}

static void datagenCmd(std::istringstream& is, bool binaryOutput = false) {
    int games = 1000;
    std::string outPath = binaryOutput ? "ceylondemon_data.bin" : "ceylondemon_data.txt";
    long long nodes = 5000;
    u64 requestedSeed = 0;
    is >> games >> std::quoted(outPath) >> nodes >> requestedSeed;

    std::ofstream out(outPath, std::ios::app | (binaryOutput ? std::ios::binary : std::ios::openmode(0)));
    if (!out) { std::cout << "datagen: cannot open " << outPath << std::endl; return; }

    std::random_device rd;
    const u64 seed = requestedSeed ? requestedSeed
        : (u64(rd()) << 32 ^ u64(rd()) ^
           std::chrono::steady_clock::now().time_since_epoch().count());
    PRNG rng(seed);

    SilentSearch = true;
    tt.resize(16); // small hash: cleared per game
    u64 totalPos = 0;
    auto t0 = std::chrono::steady_clock::now();

    struct Sample {
        std::string fen;
        int wscore;
        std::array<u8, 32> record;
    };
    std::vector<Sample> samples;
    Undo u;

    for (int g = 1; g <= games; g++) {
        tt.clear();
        clearAllHistories();
        rootPos.setFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        gameHistLen = 0;
        gameHist[gameHistLen++] = rootPos.key;

        // Random opening (8-12 plies), as specified by the clean-data plan.
        int openPlies = 8 + int(rng.rand64() % 5);
        bool ok = true;
        for (int i = 0; i < openPlies; i++) {
            MoveList ml;
            genMoves<false>(rootPos, ml);
            Move legal[256];
            int n = 0;
            for (int j = 0; j < ml.count; j++)
                if (rootPos.make(ml.list[j].move, u)) {
                    rootPos.unmake(ml.list[j].move, u);
                    legal[n++] = ml.list[j].move;
                }
            if (n == 0) { ok = false; break; }
            rootPos.make(legal[rng.rand64() % n], u);
            rootPos.commitAccumulator();
            gameHist[gameHistLen++] = rootPos.key;
        }
        if (!ok) { g--; continue; }

        // skip badly unbalanced openings
        Stopped = false;
        searchLimits = SearchLimits{};
        searchLimits.nodeLimit = 1500;
        searchStart = std::chrono::steady_clock::now();
        searchSync(rootPos);
        if (std::abs(threadData[0]->lastScore) > 500) { g--; continue; }

        samples.clear();
        double result = 0.5;
        int adjCount = 0;

        for (int ply = 0; ply < 300; ply++) {
            // game over?
            MoveList ml;
            genMoves<false>(rootPos, ml);
            int n = 0;
            for (int j = 0; j < ml.count && !n; j++)
                if (rootPos.make(ml.list[j].move, u)) { rootPos.unmake(ml.list[j].move, u); n++; }
            if (n == 0) {
                result = rootPos.inCheck() ? (rootPos.stm == WHITE ? 0.0 : 1.0) : 0.5;
                break;
            }
            int reps = 0;
            for (int j = 0; j < gameHistLen; j++) reps += gameHist[j] == rootPos.key;
            if (reps >= 3 || rootPos.halfmove >= 100 || insufficientMaterial(rootPos))
                break; // draw

            Stopped = false;
            searchLimits = SearchLimits{};
            searchLimits.nodeLimit = nodes;
            searchLimits.depthLimit = 32;
            searchStart = std::chrono::steady_clock::now();
            Move best = searchSync(rootPos);
            int score = threadData[0]->lastScore; // side-to-move pov
            int wscore = rootPos.stm == WHITE ? score : -score;

            // win adjudication: decisive score held for 4 consecutive plies
            if (std::abs(score) >= 1500) {
                if (++adjCount >= 4) { result = wscore > 0 ? 1.0 : 0.0; break; }
            } else adjCount = 0;

            if (!rootPos.inCheck() && !rootPos.isCapture(best)
                && flagOf(best) != PROMOTION && std::abs(score) < 3000
                && (!binaryOutput || samples.size() < 256)) {
                Sample sample{binaryOutput ? std::string{} : rootPos.fen(), wscore, {}};
                if (binaryOutput)
                    sample.record = trainingRecord(rootPos, wscore, int(samples.size()));
                samples.push_back(std::move(sample));
            }

            rootPos.make(best, u);
            rootPos.commitAccumulator();
            gameHist[gameHistLen++] = rootPos.key;
        }

        // A binary shard must contain one detectable group per requested game.
        // Retry the game if every position was filtered out.
        if (binaryOutput && samples.empty()) { g--; continue; }

        const u8 encodedResult = result > 0.75 ? 2 : result < 0.25 ? 0 : 1;
        for (auto& s : samples) {
            if (binaryOutput) {
                s.record[26] = encodedResult;
                out.write(reinterpret_cast<const char*>(s.record.data()), s.record.size());
            } else {
                out << s.fen << " | " << s.wscore << " | " << result << "\n";
            }
        }
        totalPos += samples.size();

        if (g % 25 == 0) {
            out.flush();
            auto secs = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - t0).count();
            std::cout << "datagen: " << g << "/" << games << " games, "
                      << totalPos << " positions, "
                      << (secs ? totalPos / secs : 0) << " pos/s" << std::endl;
        }
    }
    out.flush();
    SilentSearch = false;
    std::cout << "datagen done: " << totalPos << " positions -> " << outPath
              << " seed " << seed << (binaryOutput ? " binary32" : " text") << std::endl;
}

static void uciLoop() {
    std::string line, token;
    while (std::getline(std::cin, line)) {
        std::istringstream is(line);
        token.clear();
        is >> std::skipws >> token;

        if (token == "uci") {
            std::cout << "id name " << resonance::ENGINE_NAME << "\n"
                      << "id author Madushan Dissanayake\n"
                      << "option name Hash type spin default 128 min 1 max 4096\n"
                      << "option name Threads type spin default 1 min 1 max 256\n"
                      << "option name Move Overhead type spin default 30 min 0 max 1000\n"
                      << "option name EvalFile type string default <embedded>\n";
            tuning::printUciOptions();
            std::cout << "info string "
                      << (resonance::weightsLoaded
                              ? std::string(resonance::NET_NAME) + " active (" + resonance::netInfo() + ")"
                              : std::string("no network loaded"))
                      << "\n";
            std::cout << "uciok" << std::endl;
        } else if (token == "isready") {
            if (searchFinished && searchThread.joinable()) searchThread.join();
            std::cout << "readyok" << std::endl;
        } else if (token == "setoption") {
            std::string name, value;
            is >> token; // "name"
            while (is >> token && token != "value") name += (name.empty() ? "" : " ") + token;
            while (is >> token) value += (value.empty() ? "" : " ") + token;
            if (name == "Hash") {
                stopSearchThread();
                tt.resize(std::clamp(std::stoi(value), 1, 4096));
            } else if (name == "Move Overhead") {
                moveOverhead = std::clamp(std::stoi(value), 0, 1000);
            } else if (name == "Threads") {
                stopSearchThread();
                Threads = std::clamp(std::stoi(value), 1, 256);
                initThreadData(Threads);
            } else if (name == "EvalFile") {
                // try the external file; if absent/invalid, keep the built-in net
                bool fromFile = !value.empty() && resonance::loadFromFile(value);
                if (!fromFile) resonance::loadEmbedded();
                rootPos.refreshAcc();
                if (fromFile)
                    std::cout << "info string " << resonance::NET_NAME
                              << " network loaded from file: " << value << std::endl;
                else if (resonance::weightsLoaded)
                    std::cout << "info string " << resonance::NET_NAME << " active ("
                              << resonance::netInfo() << ")" << std::endl;
                else
                    std::cout << "info string no network loaded" << std::endl;
            } else if (tuning::setOption(name, value)) {
                initLmrTable();
            }
        } else if (token == "ucinewgame") {
            stopSearchThread();
            tt.clear();
            clearAllHistories();
        } else if (token == "position") {
            stopSearchThread();
            setPosition(is);
        } else if (token == "go") {
            goCommand(is);
        } else if (token == "stop") {
            Stopped = true;
        } else if (token == "quit") {
            break;
        } else if (token == "datagen") {
            datagenCmd(is, false);
        } else if (token == "datagenbin") {
            datagenCmd(is, true);
        } else if (token == "bench") {
            benchCommand(is);
        } else if (token == "featurehash") {
            featureHashCommand();
        } else if (token == "eval") {
            int sigma = 0;
            int residual = 0;
            int e1 = evaluate(rootPos, &sigma, &residual);
            std::cout << "Blended score (Resonance units): " << e1 << "\n"
                      << "Normalized score (UCI cp):       " << scoreToUciCp(e1) << "\n"
                      << "Head disagreement (sigma):    " << sigma << "\n"
                      << "Interaction residual:         " << residual << std::endl;
            if (resonance::weightsLoaded) { // verify incremental accumulator against refresh
                rootPos.refreshAcc();
                int e2 = evaluate(rootPos);
                if (e1 != e2)
                    std::cout << "ACC MISMATCH: incremental " << e1
                              << " vs refreshed " << e2 << std::endl;
            }
        } else if (token == "d") {
            for (int r = 7; r >= 0; r--) {
                for (int f = 0; f < 8; f++) {
                    int sq = r * 8 + f;
                    char c = '.';
                    if (rootPos.board[sq] != NO_PIECE) {
                        c = "pnbrqk"[rootPos.board[sq]];
                        if (rootPos.colorOn(sq) == WHITE) c = char(toupper(c));
                    }
                    std::cout << c << ' ';
                }
                std::cout << '\n';
            }
            std::cout << "key " << std::hex << rootPos.key << std::dec << std::endl;
        }
    }
    stopSearchThread();
}

int main() {
    // Keep the standard streams synchronized: search output is produced by a
    // worker while UCI readiness replies come from the input thread.
    std::ios::sync_with_stdio(true);
    initTables();
    initZobrist();
    initCastlePerm();
    initSearch();
    tt.resize(128);
    // Deterministic default: use the exact network compiled into this binary.
    // An external network is loaded only through the explicit EvalFile option.
    resonance::loadEmbedded();
    rootPos.setFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    gameHist[gameHistLen++] = rootPos.key;
    uciLoop();
    return 0;
}
