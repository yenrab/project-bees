# Spike S11: Balancing through `migrate_actor()`

**Status:** run 2026-09-17 on `silica-999977-macos-applesilicon` (10 cores: 4 performance, 6
efficiency, per `get_performance_cores`/`get_efficiency_cores`). **Answers the cost and ordering
questions; the "takes effect at the next dispatch boundary" question is not observable on macOS.**
**Trials:** `trials/s11_balancing_migrate/`.

| Question | Result |
| --- | --- |
| Cost of `migrate_actor(ref, core)` | **~0.2 µs** (400,000 calls in ~0.1 s of wall time beyond the run's fixed cost). On macOS it only re-requests an affinity hint for the actor's thread. |
| Message order across a move | **Holds.** An actor receiving 100,000 numbered messages while being migrated every 100 messages round-robin over the 10 cores sees every number in order (`order_across_migrations`). |
| Does a move take effect at the next dispatch boundary? | Not observable: every actor is its own pthread (S-6 not landed), so a "move" is a thread-affinity hint and there is no dispatch scheduler to observe. This question returns with S-6 and on the ESP32-S3 (S-23). |
| Can BEES balance from its own work counts alone? | Nothing in the runtime reports load, so yes by necessity; the counting is cheap next to a 0.2 µs move. Whether the hints move anything is up to macOS. |
| API shape | `spawn(state, behaviour, core)` pins at spawn; `migrate_actor(ref, core)`; `move(ref, from, to)` also exists. The core argument must be a bound value: an expression there faults (SD-19). |

**Consequences.** A6's balancer can be written now against this API and measured for real only once
S-6 gives Silica a per-core scheduler; until then the hosted balancer is a hint generator, which
is what roadmap §4.10 already says an OS-hosted pin is.
