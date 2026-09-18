# Spike S6: Terms and heap

**Status:** run 2026-09-17 on `silica-999977-macos-applesilicon`. **Partly failed:** a term as a
recursive tuple in a region works, in actor state and in messages, but **no region can be reclaimed
while an actor lives**, so evacuation leaks until S-15, and message regions leak (SD-7).
**Trials:** `trials/s06_terms_and_regions/`; measurements from the scratch programs described below.

## 1. Questions

1. Can `bees_term` be a recursive tagged tuple in a region held in actor state, and can it grow
   across dispatches?
2. Can a term travel in a message?
3. Can a region be released today? What does an evacuation cost?

## 2. Results

| # | Result |
| --- | --- |
| 1 | **Yes.** State `{ r: region(L, normal), root: ref?(L, normal, rec) }`; a cell is `alloc_rec(state.r, (:int, v, prev))`; the actor returns the same region in its new state. Two calls grow a two-node chain (`term_in_state_across_calls`). |
| 2 | **Yes,** by moving a region inside the message (§12.1.6): `{ r: region(L, normal), root: ref?(L, normal, rec) }` as the `call` payload; the receiver walks the chain (`term_in_message_region`). |
| 3 | **No release while the actor lives.** A region dropped at function scope exit *is* freed (100,000 regions: 9 MB). A region moved into a message is never freed after the handler (100,000 casts: 550 MB, SD-7). A state region replaced by a fresh one on every dispatch, which is exactly BEES's evacuation, is never freed either (100,000 dispatches: 551 MB, `evacuation_leaks_until_s15`). This is the S-15 gap, now measured at ~5.5 KB per region. |
| 3, cost | An evacuation of one cell into a fresh region, including the allocation, costs about 1 µs (100,000 in ~0.1 s wall). |

## 3. What the term must look like (constraints found on the way)

- Optional recursive references are written `ref?(L, normal, rec)` everywhere; the spelled-out
  form is rejected (SD-4).
- A `ref` becomes a `ref?` only through an `alloc_rec` argument or a function whose return type is
  `ref?(L, normal, rec)`; a binding cannot be annotated `ref?` (SD-5). BEES keeps a one-line
  `some_term(c) -> ref?(L, normal, rec)`.
- Lifetime names are nominal (SD-6): every unit calls the term lifetime `L`, every term region is
  `region(L, normal)`, and a behaviour that receives a message region and holds a state region has
  both typed `L`, which the compiler accepts.
- A BEAM process is a **cast-only** actor returning `(:no_reply, state)`; a `cast` to a
  `(:reply, …)` behaviour with a record state faults (SD-8).
- A function's own `mem(normal)` effect does not propagate to callers: a sequence that only calls
  such a function must not declare `mem(normal)` (E3010).

## 4. Consequences

- **S-15 is a 1.0 blocker for A1, as the ledger says,** and the interim ("short-lived processes
  only") is confirmed: a long-lived process's memory grows by one region per dispatch.
- **Messages need a decision (SD-7):** either Silica reclaims message regions, or BEES carries
  message terms region-free. Until then every message leaks ~5.5 KB.
- Contract item 1 gains the `L` naming rule and the `ref?(L, normal, rec)` form.
