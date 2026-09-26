# CeylonDemon 3.3 release record

Released 26 September 2026. 3.3 keeps the 3.2 engine and its Resonance v12
400M network and adds two search changes. It is the strongest CeylonDemon
measured to date.

## Changes (all in `src/search.h`)

- **Singular extensions from depth 6** (previously 8), so forcing lines are
  resolved in shallower subtrees.
- **Reductions for captures and checks.**
  - Checking quiet moves now enter late-move reduction, one ply less than
    other quiets; before, they were never reduced.
  - Late non-checking captures get half the quiet reduction, adjusted by
    capture history (`capHist / 6000`, clamped to ±2), one ply less at
    was-PV nodes and one more at expected cut nodes.

## Network

Unchanged from 3.2: Resonance v12, trained from random initialization on
400,746,429 CeylonDemon self-play positions (396,493,884 training, 4,252,545
held-out validation), epoch 8 selected on validation loss
`0.010458274903805277`.

- File: `networks/ceylondemon-3.1-resonance-v12-400m-full-scratch-e8.aa`
- SHA-256: `38B398F22E792AD4B4B7A79920AEF6B7574011F43E1F6751BFA15B4750CCCE57`

## Validation

- Start-position perft 5: `4,865,609`. Kiwipete perft 4: `4,085,603`.
- Depth-10, 32-position bench (`bench 10 16`): `1,097,453` nodes, signature
  `a218a8c01ea3aa9c`, identical for the tested candidate and the AVX2 and BMI2
  releases. (3.2: `872,024`, `7aa2ea9d3f3145bf`.)
- Individual matches vs 3.2 at 4+0.04, Threads=1, Hash=16 MB, UHO openings,
  500 games each:

| Experiment | W | L | D | Elo | 95% interval | Result |
|---|---|---|---|---|---|---|
| Singular extensions from depth 6 | 161 | 126 | 213 | +24.4 | +1.3 to +47.6 | kept |
| Capture/check reductions | 145 | 124 | 231 | +14.6 | −7.7 to +37.0 | kept |
| Material + previous-move correction tables | 149 | 143 | 208 | +4.2 | −19.1 to +27.5 | dropped |

  One singular-extension win came from 3.2 overrunning its clock by 129 ms.
  The two kept changes were not tested together head-to-head before release.

## How strong is it? 

The total accumalative SPRT gains though versions 2.1 to 3.3 is around 400 points. 

In a 100 game (1 min + 1 sec) test against 10 other engines ( 3300 - 3750 ccrl blitz)  on my laptop (Intel Core i7- 6600U , 2.60 GHz), the current 3.3 version had a performance 
 rating of about **3450**. That is the best result of any CeylonDemon version
so far. For comparison, the best human players are rated around 2820 and best chess engine on the planet , which is stockfish 19 rated around 3650 on Long time controls and about 3780 on short time controls. 


## Release binaries

| File | Size (bytes) | SHA-256 |
|---|---|---|
| `CeylonDemon-3.3-x86-64-avx2.exe` | 13,494,302 | `8EC9047E059F55D0698F9DE22E9653C6AE0B89AB48DED209C5385F3BF6A1CC81` |
| `CeylonDemon-3.3-x86-64-bmi2.exe` | 13,493,790 | `DB8C46A789A4D6A3957B11A08F625615C50234277197943D6641577AD48D6AAE` |

The AVX2 build is the portable default. The BMI2 build is intended for CPUs
with fast BMI2/PEXT (Intel Haswell and later, AMD Zen 3 and later).

A rebuild from this source with `.\build.ps1` or `make` is not byte-identical
to these files, so its SHA-256 will differ, but it reports the same UCI
identity, perft counts and bench signature.
