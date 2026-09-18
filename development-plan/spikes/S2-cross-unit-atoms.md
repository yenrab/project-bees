# Spike S2: Cross-unit atoms

**Status:** run 2026-09-17 on `silica-999977-macos-applesilicon`. **Failed as first written**, which
confirmed SD-3 (S-1); **passes under D30**, the decision it led to.
**Trial:** `trials/s02_cross_unit_atoms/` (units `src/shim/bees_spike_atom_a.silica`, `bees_spike_atom_b.silica`).

## 1. Question

Does a Silica atom minted in one compilation unit compare equal in another, as a return value, a
tuple element, a `case` label and an `==` operand? Does a generated atom lookup (a `case` over atom
literals) return the right spelling for an atom from another unit?

## 2. Result

No. Unit A returns `:zebra`, `:apple`, `:mango`; unit B classifies them as 1, 2, 3 (apple, mango,
zebra) instead of 3, 1, 2; `m == :mango` in a third unit is false; the lookup spells `:mango` as
"zebra".

The assembly shows why. Every unit carries its own atom table: 23 runtime atoms
(`:efficiency`, `:performance`, `:ok`, `:normal`, `:language_error`, …) followed by the unit's own
atoms numbered **by first appearance in that unit**, and an atom literal is emitted as that number
(`MOV X0, #25`). Unit A numbers zebra=23, apple=24, mango=25; unit B numbers apple=23, mango=24,
zebra=25. Nothing at link time or run time reconciles them. The spec's one program-wide table
(§4.1.7) is not what the fixed point implements.

## 3. Consequences for BEES

- Compiled BEAM code exchanges atoms across units constantly (every module is a unit), so nothing
  in the plan works until atom identity is program-wide.
- **S-1 is the first Track S item after S-5** in the order roadmap §3 gives. It is a defect against
  the spec, not a new feature, so the filing is a reproducer plus a pointer to §4.1.7.

## 4. Resolution: D30

Only the number crosses a unit boundary, so values agree program-wide when every atom literal is
written in one unit. `bees_config atoms` generates that unit, `bees_atoms`, from the units'
manifests (`*.atoms`): one accessor per atom, `spelling`, `index` and `from_spelling`. Units A
and B were rewritten to hold no atom literal: A returns `bees_atoms@a_mango()`, B switches on
`bees_atoms@index(a)` and compares with `==`. The trial now prints the expected
`3 1 2 / true / mango / 2 7 / true true true / true false`, and `bees_config check` reports any
BEAM atom literal written outside `bees_atoms`.

What D30 does not cover: `case` patterns on BEAM atoms outside the unit (use `index`), and
stdlib units that exchange atoms through callbacks (`wbt_map`'s comparator), which still rely on
matching first-appearance order.

## 5. The interim considered before D30 (not adopted)

Because numbering is by first appearance, a program whose every unit mentions every atom **in the
same order before any other atom** gets identical numbers in every unit. `bees_config` already
generates the program-wide atom lookup from the compilers' manifests (roadmap §4.6); it could also
emit an **atom prelude** — one function listing every program atom in canonical order — that is
placed at the top of every unit before compilation:

- for units the compilers emit, the contract would require the prelude as the first item;
- for BEES's own units, a build step would inject it.

Atoms stay Silica's, and BEES still has no atom table; only the numbering is coordinated. The
prelude retires when S-1 lands. Costs: a source-injection step, and every unit pays for the whole
program's atom table. Not adopted: it needs a decision (see the roadmap's open items).

The recorded golden is the correct output under D30.
