# Spike S1: Trait mapping

**Status:** run 2026-09-17 on `silica-999977-macos-applesilicon`. **Blocked by SD-10** after the
first question; the split-`gen_server` and state-machine questions are not yet run.
**Trials:** `trials/s01_trait_mapping/`.

## 1. Question 1: can a `Supervisor` host a child with `bees_term` state and messages?

The child of a BEES supervisor is a cast-only actor whose state is a record holding a region and
a term root, `{ r: region(L, normal), root: ref?(L, normal, rec), … }` (spike S6), with
`(:no_reply, state)` returns (SD-8).

**Not today.** Every child state that is not a single `int64` or `actor_ref` is passed wrongly
through the supervisor path:

| Child state | Through plain `spawn` + `call` | Through `spawn_registered_supervisor` child spec |
| --- | --- | --- |
| `int64` | works | works (all 105 Silica supervisor trials use this) |
| `actor_ref` | works | works |
| `(int64, int64)`, read with a tuple pattern | 35 (correct) | **16 (garbage)** |
| `{ a: int64, b: int64 }`, reading `state.a` | 35 (correct) | **fault in the worker** |
| `{ r: region(L, normal), root: ref?(L, normal, rec), collector: actor_ref }` | works (S6) | **fault in `init`** |

Passing a record state through untouched (`(:reply, msg, state)`) does not fault, so the words
are being handed over but not laid out as the behaviour expects. Recorded as **SD-10**.

## 2. What this blocks

- D18 (every actor has a supervisor) cannot be implemented for BEES processes until SD-10 is
  fixed, because a process's state is a record with a region. The per-core spawn supervisors
  (A2, A6) and every `supervisor` mapping (A4) sit on the same path.
- The interim would be unsupervised `spawn` for PoC processes, which violates D18, so it is not
  adopted here; it needs a decision.

## 3. Not yet run

- Whether a split `gen_server` (a call-only and a cast-only actor) keeps OTP semantics.
- The shape Silica's state-machine trait must have (S-17). Both wait on a supervisor that can
  hold the child.
