# Spike S13: Lifecycle cost

**Status:** partly run 2026-09-17 on `silica-999977-macos-applesilicon`. Message copies are measured;
supervised spawn and the exit hub wait on SD-10 and S-28; the yield-versus-mailbox-depth measurement
waits on the dispatch-budget helper (A6).
**Trial:** `trials/s13_lifecycle_cost/message_copy_by_size.silica` (sizes edited for the runs below).

## 1. Message cost by term size

The sender builds a list of N ints in a fresh region and casts it; the receiver copies it into its
heap (evacuation) and sums it.

| N cells | messages | wall beyond the 0.3 s idle wait | CPU | peak RSS |
| --- | --- | --- | --- | --- |
| 1 | 20,000 | ~0.3 s | 0.10 s | 117 MB |
| 10 | 20,000 | ~0.35 s | 0.11 s | 151 MB |
| 100 | 20,000 | ~0.5 s | 0.37 s | 806 MB |
| 1,000 | 2,000 | ~0.5 s | 0.34 s | 758 MB |

So building plus copying costs **~0.2 µs per cell** (2,000,000 cells in ~0.4 s), and a message's
fixed cost is ~5 µs of wall time, most of it the region allocation. Memory is the story: every
message region (SD-7) and every replaced heap region (S-15) stays allocated, ~400 bytes per cell
across sender and receiver, so 2,000,000 cells cost 800 MB.

## 2. Found on the way

- **SD-21**: aggregate returns leak stack per call; the first `bees_term` (records returned
  everywhere) overflowed the receiver's 8 MB stack after ~35,000 cells. The API now returns single
  references.
- **SD-22**: a message region bound to a local is freed at scope exit despite the cast; builders now
  take the fresh heap straight from `new_heap()`.

## 3. Not yet measured

- Spawn through a per-core spawn supervisor (D18): blocked by SD-10.
- Exit through the supervisor and the hub: needs S-28 (spike S7, a Silica branch).
- A yield with a mailbox of depth M: needs the dispatch-budget helper and the yield protocol (A6);
  the save-queue scan it would pay is `bees_recv@select`, O(M) cells at ~0.2 µs each.
