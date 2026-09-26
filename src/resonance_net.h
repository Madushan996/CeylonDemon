// SPDX-License-Identifier: GPL-3.0-or-later
// CeylonDemon — Copyright (C) 2026 Madushan Dissanayake.
// Licensed under the GNU GPL v3 or later; see LICENSE.
//
// resonance_net.h — dense stack, interaction residual, sigma, and .aa loader.
//
// The forward pass, per head, per output bucket:
//
//   accumulator -> pairwise clipped product (L1 -> L1/2 per perspective,
//                  side to move first, so the net always sees "us, then them")
//               -> FC1  320 -> 16
//               -> clipped activation concatenated with its square (16 -> 32)
//               -> FC2   32 -> 64
//               -> clipped activation (64)
//               -> FC3   64 -> 1
//
// Both heads run it; the scalars are blended by a per-bucket learned gate. A
// rank-8 product branch then reads the two 64-value hidden contexts and adds a
// bounded correction. The experts' absolute scalar difference remains sigma.
//
// Every rescale rounds to nearest rather than truncating. An arithmetic shift
// alone biases every activation downwards and the bias compounds through the
// stack; adding half a unit first keeps the quantised network within a couple
// of internal units of the float one it was trained as.
//
// SCALE NOTE: these are the network's internal units, where a pawn is about
// 208 — not the ~90 the search margins were originally written against. The
// network defines the scale, so the search margins are rescaled to it (see
// ev() in search.h) rather than rescaling the network output.
#pragma once
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include "resonance_acc.h"
#include "resonance_arch.h"
#include "resonance_embedded.h"
#include "resonance_features.h"
#include "resonance_simd.h"
#include "types.h"

namespace resonance {

// Evaluations are kept inside this band so they never collide with mate scores.
//
// 2.0 CHANGE — the last inherited constant is gone. Through 1.11 this was
// 31506, carried from the 1.6 tree, where it was Stockfish's evaluation clamp
// (VALUE_TB_WIN_IN_MAX_PLY - 1 = 32000 - 246 - 1 - 246 - 1). That value is
// only safe under Stockfish's score scale, where mate is 32000 and the mate
// threshold is 31754, so 31506 sits below both.
//
// It is not safe here. This engine uses MATE = 30000, MATE_BOUND = 29000 and
// INF = 31000, so a clamp at 31506 sat *above* all three: an evaluation at the
// bound would have exceeded INF and been reported as a mate score by
// scoreString() and shifted by ply in scoreToTT(). No reachable position
// produces an evaluation near it with the shipped network, so nothing observed
// ever depended on it, but the relationship was inverted.
//
// Derived from this engine's own constants instead: strictly inside the mate
// band, so an evaluation can never be mistaken for a mate score by
// construction rather than by the coincidence of a foreign scale.
constexpr int EVAL_LIMIT = MATE_BOUND - 1; // 28999

// ----------------------------------------------------------------------------
// Dense weights, per head and per output bucket.
// ----------------------------------------------------------------------------

struct DenseWeights {
    alignas(64) int8_t  fc1Weight[HEAD_NB][OUTPUT_BUCKETS][FC1_OUT * FC1_IN];
    alignas(64) int32_t fc1Bias  [HEAD_NB][OUTPUT_BUCKETS][FC1_OUT];

    alignas(64) int8_t  fc2Weight[HEAD_NB][OUTPUT_BUCKETS][FC2_OUT * FC2_IN];
    alignas(64) int32_t fc2Bias  [HEAD_NB][OUTPUT_BUCKETS][FC2_OUT];

    alignas(64) int8_t  fc3Weight[HEAD_NB][OUTPUT_BUCKETS][FC3_IN];
    int32_t             fc3Bias  [HEAD_NB][OUTPUT_BUCKETS];

    // Blend gate, in 1/256ths of the AEGIS head, per output bucket.
    int16_t gate[OUTPUT_BUCKETS];
};

inline DenseWeights dw;

// Resonance v12's interaction branch. Arrays are stored per output bucket so
// the residual can specialize by material phase. They can be trained alone or
// jointly with the parent evaluator, as in the CeylonDemon 2.8 checkpoint.
struct InteractionWeights {
    alignas(64) int8_t  aegisProjection[OUTPUT_BUCKETS][INTERACTION_RANK * INTERACTION_IN];
    alignas(64) int32_t aegisBias      [OUTPUT_BUCKETS][INTERACTION_RANK];
    alignas(64) int8_t  lanceProjection[OUTPUT_BUCKETS][INTERACTION_RANK * INTERACTION_IN];
    alignas(64) int32_t lanceBias      [OUTPUT_BUCKETS][INTERACTION_RANK];
    alignas(64) int8_t  outputWeight   [OUTPUT_BUCKETS][INTERACTION_RANK];
    int32_t             outputBias     [OUTPUT_BUCKETS];
};

inline InteractionWeights iw;

// Runtime-only packing; serialized network and learned values stay intact.
alignas(64) inline int8_t packedFc2[HEAD_NB][OUTPUT_BUCKETS][FC2_OUT * FC2_IN];
inline void packDenseWeights() {
    for (int h = 0; h < HEAD_NB; ++h)
        for (int b = 0; b < OUTPUT_BUCKETS; ++b)
            for (int o = 0; o < FC2_OUT; o += 8)
                for (int i = 0; i < FC2_IN; i += 4)
                    for (int lane = 0; lane < 8; ++lane)
                        for (int k = 0; k < 4; ++k)
                            packedFc2[h][b][o * FC2_IN + i * 8 + lane * 4 + k]
                                = dw.fc2Weight[h][b][(o + lane) * FC2_IN + i + k];
}

// Eight FC2 outputs occupy eight int32 lanes. Clip and consume in FC3,
// avoiding scalar horizontal reductions and intermediate activation arrays.
// maddubs cannot saturate: inputs <=127, int8 weights => [-32512,32258].
inline int32_t fusedTail(const uint8_t* input, int h, int b,
                         uint8_t* policyContext) {
#if defined(RESONANCE_AVX2)
    const __m256i ones = _mm256_set1_epi16(1);
    const __m256i round = _mm256_set1_epi32(1 << (WEIGHT_SCALE_BITS - 1));
    const __m256i zero = _mm256_setzero_si256();
    const __m256i cap = _mm256_set1_epi32(ACTIVATION_CLIP);
    __m256i total = zero;
    for (int o = 0; o < FC2_OUT; o += 8) {
        __m256i value = _mm256_load_si256(
            reinterpret_cast<const __m256i*>(dw.fc2Bias[h][b] + o));
        for (int i = 0; i < FC2_IN; i += 4) {
            int32_t word;
            memcpy(&word, input + i, sizeof(word));
            const __m256i x = _mm256_set1_epi32(word);
            const __m256i w = _mm256_load_si256(
                reinterpret_cast<const __m256i*>(packedFc2[h][b] + o * FC2_IN + i * 8));
            value = _mm256_add_epi32(value,
                _mm256_madd_epi16(_mm256_maddubs_epi16(x, w), ones));
        }
        value = _mm256_srai_epi32(_mm256_add_epi32(value, round), WEIGHT_SCALE_BITS);
        value = _mm256_min_epi32(_mm256_max_epi32(value, zero), cap);
        if (policyContext) {
            alignas(32) int32_t lanes[8];
            _mm256_store_si256(reinterpret_cast<__m256i*>(lanes), value);
            for (int lane = 0; lane < 8; ++lane)
                policyContext[o + lane] = uint8_t(lanes[lane]);
        }
        const __m128i w8 = _mm_loadl_epi64(
            reinterpret_cast<const __m128i*>(dw.fc3Weight[h][b] + o));
        total = _mm256_add_epi32(total,
            _mm256_mullo_epi32(value, _mm256_cvtepi8_epi32(w8)));
    }
    return dw.fc3Bias[h][b] + simd::horizontalSum(total);
#else
    alignas(64) int32_t hidden[FC2_OUT];
    simd::affine(hidden, input, dw.fc2Weight[h][b], dw.fc2Bias[h][b], FC2_IN, FC2_OUT);
    int32_t out = dw.fc3Bias[h][b];
    for (int i = 0; i < FC2_OUT; ++i) {
        const int y = std::clamp((hidden[i] + (1 << (WEIGHT_SCALE_BITS - 1)))
                                  >> WEIGHT_SCALE_BITS, 0, ACTIVATION_CLIP);
        if (policyContext) policyContext[i] = uint8_t(y);
        out += y * int32_t(dw.fc3Weight[h][b][i]);
    }
    return out;
#endif
}
inline std::string loadedFrom;

// ----------------------------------------------------------------------------
// Bounds-checked cursor over the network blob. The same parser serves a file
// read into memory and the copy linked into the executable, so there is exactly
// one description of the layout to keep in step with the trainer.
// ----------------------------------------------------------------------------

class Reader {
public:
    Reader(const unsigned char* data, size_t size) : p(data), end(data + size) {}
    template <typename T> bool read(T* dst, size_t count) {
        const size_t bytes = count * sizeof(T);
        if (size_t(end - p) < bytes) return false;
        memcpy(dst, p, bytes);
        p += bytes;
        return true;
    }
private:
    const unsigned char* p;
    const unsigned char* end;
};

// ----------------------------------------------------------------------------
// File format (little-endian throughout, the only byte order targeted):
//
//   u32 magic "ANTA", u32 version, u32 archHash, u32 reserved, char[64] desc
//   per frame:            ftBias[L1], ftWeight[FEATURE_DIM * L1]
//   per head, per bucket: fc1 weight+bias, fc2 weight+bias, fc3 weight+bias
//   gate[OUTPUT_BUCKETS]
//   per bucket: A projection+bias, L projection+bias, output weight+bias
//
// The policy head lives in a separate .aap file and is not read here.
// ----------------------------------------------------------------------------

inline bool loadFromMemory(const unsigned char* data, size_t size, const std::string& name) {
    weightsLoaded = false;
    if (!data || !size) return false;

    Reader in(data, size);

    uint32_t magic = 0, version = 0, hash = 0, reserved = 0;
    char description[64] = {};

    if (!in.read(&magic, 1) || !in.read(&version, 1) || !in.read(&hash, 1)
        || !in.read(&reserved, 1) || !in.read(description, 64))
        return false;

    if (magic != NET_MAGIC) {
        fprintf(stderr, "CeylonDemon: '%s' is not a network file\n", name.c_str());
        return false;
    }
    if (version != NET_VERSION) {
        fprintf(stderr, "CeylonDemon: '%s' uses unsupported network version %08X\n",
                name.c_str(), version);
        return false;
    }
    if (hash != archHash()) {
        fprintf(stderr,
                "CeylonDemon: '%s' was built for a different architecture "
                "(file %08X, engine %08X)\n", name.c_str(), hash, archHash());
        return false;
    }

    for (int h = 0; h < FRAME_NB; h++) {
        if (!in.read(ftw.ftBias[h], L1)) return false;
        ftw.ftWeight[h].resize(size_t(FEATURE_DIM) * L1);
        if (!in.read(ftw.ftWeight[h].data(), size_t(FEATURE_DIM) * L1)) return false;
    }

    for (int h = 0; h < HEAD_NB; h++)
        for (int b = 0; b < OUTPUT_BUCKETS; b++) {
            if (!in.read(dw.fc1Weight[h][b], FC1_OUT * FC1_IN)) return false;
            if (!in.read(dw.fc1Bias[h][b], FC1_OUT)) return false;
            if (!in.read(dw.fc2Weight[h][b], FC2_OUT * FC2_IN)) return false;
            if (!in.read(dw.fc2Bias[h][b], FC2_OUT)) return false;
            if (!in.read(dw.fc3Weight[h][b], FC3_IN)) return false;
            if (!in.read(&dw.fc3Bias[h][b], 1)) return false;
        }

    if (!in.read(dw.gate, OUTPUT_BUCKETS)) return false;

    for (int b = 0; b < OUTPUT_BUCKETS; ++b) {
        if (!in.read(iw.aegisProjection[b], INTERACTION_RANK * INTERACTION_IN)) return false;
        if (!in.read(iw.aegisBias[b], INTERACTION_RANK)) return false;
        if (!in.read(iw.lanceProjection[b], INTERACTION_RANK * INTERACTION_IN)) return false;
        if (!in.read(iw.lanceBias[b], INTERACTION_RANK)) return false;
        if (!in.read(iw.outputWeight[b], INTERACTION_RANK)) return false;
        if (!in.read(&iw.outputBias[b], 1)) return false;
    }

    packDenseWeights();
    loadedFrom    = name;
    ++weightsGeneration;
    weightsLoaded = true;
    return true;
}

inline std::string netInfo() {
    return weightsLoaded ? loadedFrom : std::string("<none>");
}

inline bool loadFromFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    std::vector<unsigned char> blob((std::istreambuf_iterator<char>(f)),
                                     std::istreambuf_iterator<char>());
    return loadFromMemory(blob.data(), blob.size(), path);
}

// The shipping build links the network into the executable, exactly as 1.6
// did, so a release stays a single self-contained file. Without it, fall back
// to finding the .aa on disk.
inline bool loadEmbedded() {
#if defined(CEYLON_EMBEDDED_NET)
    return loadFromMemory(ceylonNetStart, size_t(ceylonNetEnd - ceylonNetStart), "<built in>");
#else
    for (const char* p : {"ceylondemon.aa", "networks/ceylondemon.aa", "../ceylondemon.aa"})
        if (loadFromFile(p)) return true;
    return false;
#endif
}

// ----------------------------------------------------------------------------
// One head's forward pass. Returns the scalar in internal evaluation units.
// ----------------------------------------------------------------------------

inline int32_t propagate(const Accumulator& acc, int h, int us, int b,
                         uint8_t* policyContext = nullptr) {
    alignas(64) uint8_t activations[FC1_IN];
    alignas(64) int32_t fc1Out[FC1_OUT];
    alignas(64) uint8_t fc2In[FC2_IN];



    // Side to move first, so the network always sees "us, then them". Each
    // perspective contributes L1/2 pairwise products rather than L1 raw
    // activations.
    const int frame = h; // v10 accumulates each frame independently
    simd::pairwiseClipped(activations, acc.v[frame][us], L1 / 2);
    simd::pairwiseClipped(activations + L1 / 2, acc.v[frame][us ^ 1], L1 / 2);

    simd::affine(fc1Out, activations, dw.fc1Weight[h][b], dw.fc1Bias[h][b], FC1_IN, FC1_OUT);

    constexpr int ROUND = 1 << (WEIGHT_SCALE_BITS - 1);

    // First half of the next layer's input is the clipped activation; second
    // half is its square. A squared branch lets the network express "how far
    // from neutral" without spending a whole extra layer on it.
    for (int i = 0; i < FC1_OUT; i++) {
        int y = (fc1Out[i] + ROUND) >> WEIGHT_SCALE_BITS;
        y = y < 0 ? 0 : (y > ACTIVATION_CLIP ? ACTIVATION_CLIP : y);
        fc2In[i]           = uint8_t(y);
        fc2In[FC1_OUT + i] = uint8_t((y * y + 64) >> 7);
    }

    int32_t out = fusedTail(fc2In, h, b, policyContext);

    // The trainer scales the final layer so that dividing by the weight quant
    // lands directly in the engine's internal evaluation units. Integer
    // division truncates toward zero, so the rounding term carries the sign.
    return (out + (out >= 0 ? FC_QUANT / 2 : -FC_QUANT / 2)) / FC_QUANT;
}

// Project the two frozen hidden contexts into eight channels, multiply the
// paired activations, and read out a signed correction. The scalar product and
// final dot intentionally stay scalar: the rank is smaller than the 32-byte
// width required by the AVX2 pairwise helper.
inline int32_t interactionResidual(const uint8_t* aegisContext,
                                   const uint8_t* lanceContext, int b) {
    alignas(64) int32_t aegisRaw[INTERACTION_RANK];
    alignas(64) int32_t lanceRaw[INTERACTION_RANK];
    simd::affine(aegisRaw, aegisContext, iw.aegisProjection[b], iw.aegisBias[b],
                 INTERACTION_IN, INTERACTION_RANK);
    simd::affine(lanceRaw, lanceContext, iw.lanceProjection[b], iw.lanceBias[b],
                 INTERACTION_IN, INTERACTION_RANK);

    int32_t total = iw.outputBias[b];
    for (int i = 0; i < INTERACTION_RANK; ++i) {
        const int a = std::clamp((aegisRaw[i] + FC_QUANT / 2) >> WEIGHT_SCALE_BITS,
                                 0, ACTIVATION_CLIP);
        const int l = std::clamp((lanceRaw[i] + FC_QUANT / 2) >> WEIGHT_SCALE_BITS,
                                 0, ACTIVATION_CLIP);
        const int product = (a * l + 64) >> 7;
        total += int32_t(iw.outputWeight[b][i]) * product;
    }

    int32_t residual = (total + (total >= 0 ? FC_QUANT / 2 : -FC_QUANT / 2)) / FC_QUANT;
    return std::clamp(residual, -RESIDUAL_LIMIT, RESIDUAL_LIMIT);
}

// ----------------------------------------------------------------------------
// A small, exact-scope conversion guide for the elementary endings where one
// side has literally only its king and the other owns a rook or queen.
// Material already says the position is winning; this only breaks shallow
// search ties toward the standard mating plan: confine, then approach.
//
// PROVENANCE: the two constants below (64 and 16) and the confine-then-
// approach shape are carried forward from the 1.6 tree, which was
// Stockfish-derived. The pattern itself — an edge-distance term plus a
// king-proximity term — is the standard "mop-up" evaluation described in the
// general chess-programming literature and predates any one engine; the
// implementation here is written against this engine's own board accessors.
// The specific values were not re-derived by measurement in this codebase.
// Flagged in NOTICE.md rather than presented as original work.
// ----------------------------------------------------------------------------

constexpr int MOPUP_EDGE     = 64;
constexpr int MOPUP_APPROACH = 16;

inline int loneKingMopup(const u64 byColor[2], const u64 byPiece[6], int stm) {
    const auto bareKing = [&](int c) { return byColor[c] == (byColor[c] & byPiece[KING]); };

    int strong = WHITE, weak = BLACK;
    const u64 whiteHeavy = byColor[WHITE] & (byPiece[ROOK] | byPiece[QUEEN]);
    const u64 blackHeavy = byColor[BLACK] & (byPiece[ROOK] | byPiece[QUEEN]);

    if (bareKing(WHITE) && blackHeavy) {
        strong = BLACK; weak = WHITE;
    } else if (!(bareKing(BLACK) && whiteHeavy)) {
        return 0;
    }

    const int weakKing   = lsb(byColor[weak] & byPiece[KING]);
    const int strongKing = lsb(byColor[strong] & byPiece[KING]);

    const int wf = fileOf(weakKing), wr = rankOf(weakKing);
    int edgeDistance = wf < 7 - wf ? wf : 7 - wf;
    if (wr < edgeDistance) edgeDistance = wr;
    if (7 - wr < edgeDistance) edgeDistance = 7 - wr;

    // Chebyshev king distance, matching v1.6's distance<Square>.
    const int df = fileOf(strongKing) - wf, dr = rankOf(strongKing) - wr;
    const int af = df < 0 ? -df : df, ar = dr < 0 ? -dr : dr;
    const int kingDistance = af > ar ? af : ar;

    const int bonus = (3 - edgeDistance) * MOPUP_EDGE + (7 - kingDistance) * MOPUP_APPROACH;
    return stm == strong ? bonus : -bonus;
}

// ----------------------------------------------------------------------------
// Full evaluation: both heads, blended, with sigma reported.
// ----------------------------------------------------------------------------

inline int evaluate(const Accumulator& acc, const u64 byColor[2], const u64 byPiece[6],
                    int stm, int* sigma = nullptr, int* residualOut = nullptr) {
    const int b = outputBucket(popcount(byColor[WHITE] | byColor[BLACK]));

    alignas(64) uint8_t aegisContext[INTERACTION_IN];
    alignas(64) uint8_t lanceContext[INTERACTION_IN];
    const int32_t aegis = propagate(acc, HEAD_AEGIS, stm, b, aegisContext);
    const int32_t lance = propagate(acc, HEAD_LANCE, stm, b, lanceContext);

    // The residual between the defensive and offensive readings is the whole
    // point of running two heads: it is large exactly where a single scalar
    // evaluation is least trustworthy. It is reported for diagnostics and
    // future search work; the 2.8 search does not consume it.
    if (sigma) *sigma = aegis > lance ? aegis - lance : lance - aegis;

    int g = int(dw.gate[b]);
    g = g < 0 ? 0 : (g > 256 ? 256 : g);
    int32_t blended = (aegis * g + lance * (256 - g)) / 256;

    const int32_t residual = interactionResidual(aegisContext, lanceContext, b);
    if (residualOut) *residualOut = residual;
    blended += residual;

    blended += loneKingMopup(byColor, byPiece, stm);

    return blended < -EVAL_LIMIT ? -EVAL_LIMIT : (blended > EVAL_LIMIT ? EVAL_LIMIT : blended);
}

} // namespace resonance
