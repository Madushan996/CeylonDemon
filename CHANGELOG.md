# Changelog

## 3.3 - earlier singular extensions, capture and check reductions

- Try singular extensions from depth 6 instead of 8.
- Reduce checking quiet moves one ply less than other late quiets instead of
  never; give late non-checking captures half the quiet reduction, steered by
  capture history.
- Keep the 3.2 network and all other search code. Perft 5 4,865,609; Kiwipete
  perft 4 4,085,603; bench 1,097,453 nodes, signature a218a8c01ea3aa9c.
- 500 games each vs 3.2 at 4+0.04: singular depth 6 +24.4 Elo (+1.3 to
  +47.6), capture/check reductions +14.6 (-7.7 to +37.0). 100-game,
  14-anchor gate at 10+0.1: 29-54-17, approx. 3414 (3297-3535).
- Promote at the project owner's direction; see RELEASE-3.3.md.

## 3.2 - 400M network, threat history, PV-aware search, partial-iteration moves

- Embed the Resonance v12 network trained from scratch on 400,746,429
  CeylonDemon 3.0 self-play positions (first built as the 3.1 candidate).
- Split quiet history by opponent threats on the origin and destination
  squares; store a was-PV bit in the TT and reduce such nodes less.
- Add the prior-quiet fail-low bonus, a linear history bonus, reduced-depth
  futility, null-move and ProbCut TT checks, ProbCut TT stores, shallower
  aspiration fail-high re-searches, non-PV fail-high softening, an improving
  fallback after checks and child-ply killer resets.
- Play a root move already proven better inside an iteration the clock cut
  short, instead of the previous iteration's move.
- Perft 5 4,865,609; Kiwipete perft 4 4,085,603; bench 872,024 nodes,
  signature 7aa2ea9d3f3145bf.
- 500 games each at 4+0.04: search bundle vs 3.1 +21.5 Elo (-0.9 to +44.1);
  time fix on top +18.1 Elo (-4.4 to +40.7). 100-game, 14-anchor gate at
  10+0.1: 26-52-22, approx. 3409 (3293-3530).
- Promote at the project owner's direction.

## 3.0 - quiet SEE pruning, fifty-move damping, cut-node LMR

- Prune quiet moves whose SEE loses more than 25*depth^2 at depth <= 8.
- Scale the static evaluation by (200 - halfmove) / 200.
- Track expected cut nodes and reduce their late quiet moves one extra ply.
- Keep the Resonance v12 network unchanged. Perft 5 4,865,609; Kiwipete
  perft 4 4,085,603; bench 1,071,492 nodes, signature 2181c4d238e86843.
- 100-game, 14-anchor gate at 10+0.1: 24-52-24, approx. 3400 (3283-3522),
  versus 2.9 under the same conditions: 20-60-20, approx. 3344 (3224-3469).
  Head-to-head vs 2.9 at 3+0.03: +38.4 Elo over 200 games (+1.7 to +75.9).
- Promote at the project owner's direction.

## 2.9 - pawn-structure correction history

- Add a bounded pawn-structure correction table trained online from the
  difference between raw Resonance evaluations and completed search scores.
- Use corrected static evaluations for pruning and improving detection while
  retaining raw neural evaluations in the transposition table.
- Pass start-position perft 5 at 4,865,609 and Kiwipete perft 4 at 4,085,603.
- Score 25-45-30 in the required 100-game, 14-anchor gauntlet at 8+0.08 for an
  approximate local rating of 3436 (95% interval 3320-3556), versus the 2.8
  reference estimate of 3396 from 1,000 games.
- Promote at the project owner's direction. Record that the +40 point estimate
  remains uncertain and that one opposing anchor lost on time.

## 2.8 - Resonance v12 full-scratch release

- Promote the full-scratch Resonance v12 epoch-12 network trained from random
  initialization on 302,044,676 CeylonDemon self-play positions.
- Normalize UCI `score cp` output from the evaluator's approximately
  208-units-per-pawn internal scale to standard 100-centipawn units without
  changing search behavior.
- Preserve start-position perft 5 at 4,865,609 nodes and the 2.8 candidate's
  depth-10 bench identity: 1,147,364 nodes, signature `c75b5064d886c607`.
- Promote at the project owner's direction as the strongest version they have
  tested; do not describe the display-only correction as an Elo gain.

## 2.6 - clean Resonance v11 generation 3

- Replace the Generation-2 evaluator with the randomly initialized
  Generation-3 epoch-12 checkpoint trained on all 252,019,620 clean self-play
  positions from 2,966,400 CeylonDemon games across 824 source shards.
- Select epoch 12 because it achieved the run's lowest validation loss,
  `0.010348025221555006`.
- Keep the 2.5 engine architecture and search unchanged; update UCI and Windows
  branding and preserve the exact 2.5 network for rollback.
- Record 2.6 as a new-network candidate pending a controlled strength gate.

## 2.5 - king-frame caching and exact SEE reuse

- Promote the Fusion-2 candidate at the project owner's request.
- Cache king-frame accumulators, defer bucket refreshes, and reuse exact SEE.
- Keep the network and architecture unchanged; update UCI/Windows branding.
- Record the 3390 local rating estimate and its limitations in the 2.5
  release record.
- Preserve 2.4 binaries and pre-promotion source/metadata for rollback.

## 2.4 - exact-result inference and accumulator optimization

- Promote Fusion-1 after Gain SPRT H1 acceptance against 2.3: 874 games,
  +311 -165 =398, +58.59 +/- 15.37 Elo, LLR 2.97.
- Pack FC2 weights in memory and fuse FC2, clipping and FC3.
- Specialize common accumulator delta patterns outside the SIMD lane loop.
- Preserve all Resonance v11 weights, architecture and search parameters; no retraining.
- Adopt the supplied red/gold knight logo and embed a multi-resolution Windows icon.
- Rebuild AVX2, BMI2 and AVX2 tuning executables with 2.4 identity; update
  build/test defaults and archive the previous frozen champion.

## 2.3 - clean Resonance v11 generation 2

- Promote the randomly initialized Resonance v11 Generation-2 epoch-12
  checkpoint trained on the combined clean corpus of 158,465,293 positions
  from 1,843,200 CeylonDemon self-play games across 512 committed shards.
- Keep the Resonance v11 architecture and search unchanged; this release's
  strength change comes from the newly trained weights.
- Preserve trainer/engine feature mapping (`152e7ae873d6a383`), start-position
  perft depth 5 at 4,865,609 nodes, and deterministic bench at 1,179,458 nodes
  with signature `fc314cca4e0715cf` on both AVX2 and BMI2 builds.
- Pass the configured Gain SPRT against the frozen 2.2 champion after 984
  games: +408 -238 =338, 58.64%, +60.63 +/- 16.97 Elo, LOS 100%, LLR 2.95
  with H1 accepted at the +2.94 boundary.
- Fix the PowerShell release builder's case-insensitive `Output` variable
  collision so `-Tune` creates a separate tuning executable instead of
  overwriting the ordinary AVX2 build.

## 2.2 - clean Resonance v11 generation 1

- Promote the randomly initialized Resonance v11 epoch-7 checkpoint trained
  exclusively on 125,097,444 positions from 1,440,000 CeylonDemon self-play
  games. No external games, teacher network, or inherited weights were used.
- Give LANCE a vertically reversed king-bucket table while retaining AEGIS's
  defensive geometry.
- Make Resonance v11 and the clean network the default build, and update UCI
  identity and Windows resources to version 2.2.
- Preserve exact trainer/engine feature mapping (`152e7ae873d6a383`) and
  serialized inference parity (zero blend error over the verification suite).
- Pass start-position perft depth 5 at 4,865,609 nodes and deterministic bench
  at 1,210,862 nodes with signature `926948b49b0eaf7f`.
- Pass the configured Gain SPRT against the frozen champion after 616 games:
  +313 -138 =165, 64.20%, +101.50 +/- 21.73 Elo, LOS 100%, LLR 2.95 with H1
  accepted at the +2.94 boundary.

## 2.0 - provenance release: last inherited constant removed

First release of the independent line as a major version. The engine is
1.11's; this release removes the final Stockfish-derived value, corrects the
latent defect it carried, and prepares the project for public distribution.

### The EVAL_LIMIT correction

- Replace `EVAL_LIMIT = 31506` with `MATE_BOUND - 1` (28999), derived from this
  engine's own constants.
- 31506 was carried from the 1.6 tree, where it was Stockfish's evaluation
  clamp `VALUE_TB_WIN_IN_MAX_PLY - 1` (32000 - 246 - 1 - 246 - 1). That bound
  is safe under Stockfish's score scale, where mate is 32000 and the mate
  threshold is 31754.
- It was **not** safe here. This engine uses `MATE = 30000`,
  `MATE_BOUND = 29000` and `INF = 31000`, so the clamp sat above all three: an
  evaluation at the bound would have exceeded `INF` and been reported as a mate
  score by `scoreString()` and shifted by ply in `scoreToTT()`.
- No reachable position produces an evaluation near 31506 with the shipped
  network, so no observed behaviour depended on it. The relationship was
  nonetheless inverted, and is now correct by construction.
- The constant `31506` no longer appears anywhere in the shipped binary
  (verified: 2 occurrences in the 1.11 `.text`, 0 in 2.0).

### Release preparation

- Add `NOTICE.md`: project history, attribution for third-party techniques and
  toolchain components, and an explicit statement of what is and is not
  original.
- Rename internal source headers from the "NARC" working title to CeylonDemon;
  datagen default output is now `ceylondemon_data.txt`.
- Write provenance notes into the source at the points they apply
  (`resonance_features.h`, `resonance_acc.h`, `resonance_net.h`).
- Flag the lone-king mop-up constants (`MOPUP_EDGE`, `MOPUP_APPROACH`) as
  carried from the 1.6 tree and not re-derived by measurement in this codebase.
- Version strings updated in `resources.rc`, `Makefile`, `build.ps1`, and the
  UCI `id name`. Repair mojibake in `README.md` and `position.h`.

### Verification

- Evaluation output identical to 1.11 across test positions including
  forced-mate endings (KQK, KRK), confirming the removed clamp was unreachable.
- Start-position perft depth 5: exactly **4,865,609** nodes.
- Kiwipete perft depth 4: exactly **4,085,603** nodes.
- Mated-root case (`f2f3 e7e5 g2g4 d8h4`, `go depth 1`) returns
  `bestmove 0000` and exits 0.
- No Stockfish identity markers present in the binary.

Strength is unchanged from 1.11 (about -23 Elo against the supplied reference
in the authoritative 30-game gate). 2.0 is a provenance and correctness
release, not a strength release. The target of at least +50 Elo over the
reference remains open.

## 1.11 - full accumulator snapshot restore

- Save the complete Resonance accumulator in each move's undo record and
  restore it with one contiguous copy.
- Eliminate reverse feature-index calculation, scattered transformer-weight
  reads, and per-piece SIMD subtraction during unmake.
- Preserve bit-identical evaluation and fixed-node search behavior; only the
  CPU work required to restore a position changes.
- Preserve the v1.10 board representation, search architecture, bounded check
  extension, time management, and Lazy SMP implementation.
- The optimization is original CeylonDemon work. No Stockfish code, Stockfish
  network, external policy, or additional NARC Next section was added.

### Verification and strength gate

- Fixed-node depth, seldepth, score, node count, PV, and best move were
  identical to v1.10 across ten opening, tactical, and checked positions.
- Start-position perft depth 5 remained exactly **4,865,609** nodes.
- Alternating pinned benchmarks measured approximately **+4.2%** median perft
  throughput and **+8.85%** median search throughput.
- In the authoritative one-thread, 128 MB, 30-game 10s+100ms gate over 15
  paired-colour openings, 1.11 scored **14.0/30** (+5-7=18), approximately
  **-23 Elo** against the supplied reference.
- The 1.10 champion scored **11.5/30** (+1-8=21), approximately **-83 Elo**;
  the promotion gains 2.5 points and about **60 Elo** by the gate estimates.

Version 1.11 is promoted as the new intermediate champion. The final target of
at least +50 Elo over the reference remains open.

## 1.10 - bounded forcing-check extension

- Extend forcing checks by one ply near the frontier, capped at two check
  extensions per line.
- Use the larger of the check and singular extensions, preventing them from
  stacking on the same move.
- Preserve the v1.9 evaluator, quiescence scale correction, PVS architecture,
  time management, and Lazy SMP implementation.
- The implementation is original CeylonDemon work. No Stockfish code, network,
  external policy, or additional NARC Next section was added.

### Strength gate

Single thread, 128 MB hash, 30 games, 10s+100ms, 15 paired-colour openings
against the supplied CeylonDemon 1.6 executable:

- 1.9 champion: **10.0/30** (+2-12=16), approximately **-120 Elo**.
- 1.10 candidate: **11.5/30** (+1-8=21), approximately **-83 Elo**.
- Measured relative improvement: **+1.5 points**, approximately **+37 Elo**.

One candidate game was lost on time. Version 1.10 is nevertheless promoted
because its completed authoritative score is strictly better. The final +50
Elo-over-reference requirement remains open.

## 1.9 - quiescence scale correction

- Convert quiescence delta-pruning capture gain and its safety margin from the
  100-units-per-pawn SEE scale to Resonance's approximately 208-units-per-pawn
  evaluation scale before comparing against alpha.
- Preserve the 1.8 board, evaluator, principal search architecture, time
  management, and Lazy SMP implementation.
- The change is original CeylonDemon work. No Stockfish code, Stockfish network,
  external policy file, or additional NARC Next section was added.

### Strength gate

Single thread, 128 MB hash, 30 games, 10s+100ms, 15 paired-colour openings
against the supplied CeylonDemon 1.6 executable:

- 1.8 baseline: **4.5/30** (+0-21=9), approximately **-301 Elo**.
- 1.9 candidate: **10.0/30** (+2-12=16), approximately **-120 Elo**.
- Measured relative improvement: **+5.5 points**, approximately **+181 Elo**.

Version 1.9 is promoted as the new intermediate champion. The final requirement
to score at least +50 Elo over the reference remains open.

## 1.8 — clock and Lazy SMP promotion

- Use the full increment in normal time allocation and make the nominal base
  budget the soft target. This removes persistent self-imposed time odds while
  retaining the existing hard flag-safety reserve.
- Keep Lazy SMP available down to 50 ms hard budgets. Helper threads diversify
  root ordering and contribute shared-TT work; only the deterministic main
  thread may select the final move.
- The helper root diversification is a small, explicitly permitted borrowing
  from NARC Next 4.2. The main-result authority and clock changes are original
  CeylonDemon work. No Stockfish code or network was added.

### Strength gate

Single thread, 128 MB hash, 20 games, 10s+100ms, paired colours against the
supplied CeylonDemon 1.6 executable:

- 1.7 baseline: **4.0/20** (+0-12=8), approximately **-241 Elo**.
- 1.8 candidate: **8.0/20** (+0-4=16), approximately **-70 Elo**.
- Measured relative improvement: approximately **+171 Elo**.

Version 1.8 is promoted as an intermediate champion. The final requirement to
score at least +50 Elo over the reference remains open.

## 1.7 — foundation rebuild

**CeylonDemon 1.7 replaces the engine's entire foundation and relicenses the
project under the GPLv3 with full source.**

Versions through 1.6 were built on a Stockfish-derived base: board
representation, move generation, the hand-crafted evaluation, and much of the
search framework. 1.7 discards that and rebuilds on an independently written
codebase.

### Evaluation is unchanged

The Resonance v10 network and its forward pass carried across exactly. This was
verified rather than assumed, against the shipped 1.6 binary:

- **3000 positions** from random legal games — blended score and sigma
  identical in every one.
- **400 move sequences** played through make/unmake, containing 184 castlings,
  73 promotions, 74 en passants and 2443 king moves — identical throughout,
  with zero incremental-versus-refresh mismatches.
- **Perft** exact: startpos depth 5 (4,865,609), Kiwipete depth 4 (4,085,603),
  and an endgame at depth 6 (11,030,083).
- **All 98,304** combinations of perspective, king square, piece colour, piece
  type and square produce identical feature indices.
- **SIMD kernels** bit-identical across 200 randomised trials each, with the
  scalar fallback matching the AVX2 path exactly.

Existing `.aa` networks for Resonance v10 load unchanged. Older Resonance
generations (v1–v9) do not; 1.7 targets v10 only.

### Changed

- Feature indexing adapted to the new foundation's piece numbering.
- Accumulators update eagerly in make/unmake rather than lazily through a
  state chain. A king move refreshes only the two frames its king anchors, and
  only when the bucket or mirror side actually changes.
- Pruning margins rescaled to the evaluator's units. The previous foundation's
  margins were tuned against a network on a different scale and would otherwise
  have pruned roughly 2.4× too aggressively. **These are a calculated starting
  point, not a measured one.**

### Removed

- The hand-crafted evaluation fallback. The network is always embedded, so it
  had no reachable use case.
- The root policy prior. Resonance's policy head lives in a separate file that
  1.7 does not load yet; it was disabled by default previously in any case.
- Support for Resonance v1–v9 network files.

### Not yet ported

- **Sigma-adaptive search.** Sigma is computed and reported, but the search
  does not consume it. 1.6 used it to widen aspiration windows, soften late
  move reductions and inflate pruning margins where the two frames disagreed.
- **Confidence-weighted correction history.**
- A bucket cache to make accumulator refreshes cheaper.

### Status

1.7 has **not been strength-gated**. Its evaluation matches 1.6 exactly; its
search does not. Treat this as a foundation release and measure before assuming
parity.
