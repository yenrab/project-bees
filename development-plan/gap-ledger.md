# BEES Gap Ledger

This ledger lists every BEAM capability BEES has to account for. For each one it records the Silica status today,
who closes the gap, and which milestone closes it. It is the checklist behind the [roadmap](roadmap.md)'s
definition of 1.0: a row marked **1.0** is closed only by trials, or by an explicit move out of scope in the
README.

**Silica status** (at Apple Silicon fixed point 1, commit `8aff9cdc5`):

- **Tested**: implemented, and covered by Silica trials.
- **Stub**: typed by the compiler, but the runtime does nothing.
- **Spec**: specified, not implemented.
- **Absent**: neither specified nor implemented.

**Class**:

- **U**: upstream. Silica owns it; BEES tracks it as a Track S item.
- **B**: BEES owns it permanently.
- **E**: the BEES native edge. Temporary C behind `dangerous_bees_*` wrappers.
- **—**: Silica already provides it, and BEES uses it as-is.

Spec section numbers refer to `silica/compiler/silica-compiler/design_documents/silica-specification.md`.

---

## 1. BEAM capability ledger

### 1.1 Processes and lifecycle

| Capability | Silica today | Class | Closed by | Target |
| --- | --- | --- | --- | --- |
| Spawn (plain, registered, pinned to a core) | Tested (`spawn`, `spawn_registered`, third argument `core_id`; §15.1.1) | — | — | — |
| Lightweight processes (not OS threads), massive counts | Spec §15.1.2.2; the implementation is one pthread per actor | U (S-6) | C2 | 1.0 |
| Process identity usable across nodes (`pid` with node, serial, creation) | Absent (`actor_ref` is an opaque local handle) | B | R0.3 contracts, B3 | 1.0 |
| Links | Stub (`_silica_rt_actor_link` returns `:ok`); spec §15.4.8.4–5 | U (S-2) | A1 | 1.0 |
| Monitors, `DOWN` messages | Stub; spec §15.4.8.6 `(:down, monitor_ref, actor_ref, failure_reason)` | U (S-2) | A1 | 1.0 |
| Trap exits (exits delivered as messages) | Absent: only supervisors trap (§15.4.8.5) | B | A1 | 1.0 |
| `exit/2` and explicit exit reasons | Absent: `(:explicit, atom)` has no producer (§15.4.11.2) | U (S-9) for the primitive, B for the API | A1 | 1.0 |
| Explicit, orderly self-stop | Underspecified (§15.1.2.1) | U (S-9) | A1 | 1.0 |
| `kill` (untrappable) | Tested as `kill_abnormal` | — | — | — |
| Supervisors (three strategies, restart intensity, dynamic children) | Tested (`Supervisor` trait, `call_supervisor`; §15.4.8–13) | — | — | — |
| Crash containment and crash reports | Tested (guarded faults, `FailureReporter`; §15.4.4–13) | — | Wired into logging in A4 | — |
| `is_process_alive`, `process_info`, `processes()` | Absent | B (needs S-8 for mailbox data) | A4 | 1.0 |
| Process dictionary | Absent | Declined (see §5) | — | — |

### 1.2 Messaging

| Capability | Silica today | Class | Closed by | Target |
| --- | --- | --- | --- | --- |
| Asynchronous send, synchronous call with reply | Tested (`cast`, `call`, `(:reply, v, s)`, `(:no_reply, s)`) | — | — | — |
| FIFO order between a pair of processes | Tested | — | — | — |
| Call timeouts | Contradictory spec (§16.1.1 vs §22.4); none implemented | B via `bees_timer` (the upstream clarification is S-9) | Stage 1, A1 | 1.0 |
| Receive with timeout (`after`) | Absent: user `recv()` is forbidden (§15.1.2) | B, as behaviour-level timeouts returned from callbacks | A1 | 1.0 |
| Selective receive | Absent by design | Declined; behaviours replace it (see §5) | — | — |
| Heterogeneous messages (one process, many message shapes) | Inline unions partly work; variants are Silica roadmap chunk 5 | U (S-10) for variants; B for the `bees_envelope` convention | R0.3, then adopt S-10 | 1.0 |
| Mailbox length, bounded mailboxes, send backpressure | Absent (unbounded mailbox, no introspection) | U (S-8) | A4 | 1.0 |
| Message isolation (copying) | A type-system claim; the runtime passes pointers | — (upstream correctness) | — | — |

### 1.3 Time

| Capability | Silica today | Class | Closed by | Target |
| --- | --- | --- | --- | --- |
| Monotonic clock | Absent (only wall-clock `current_time()`, and that is unimplemented) | E now, U (S-3) later | Stage 1 | 1.0 |
| `send_after`, `start_timer`, `cancel_timer`, `read_timer` | Absent | B (heap-based timer service) | Stage 1, A1 | 1.0 |
| `sleep` | Absent (the runtime's internal `poll(NULL,0,100)` only) | E now, U (S-3) later | A1 | 1.0 |
| gen_statem state and event timeouts | Absent | B | A2 | 1.0 |

### 1.4 Behaviours and applications

| Capability | Silica today | Class | Closed by | Target |
| --- | --- | --- | --- | --- |
| gen_server | Partial: a behaviour is call-only or cast-only, never both (§16.2.6.1) | B (`bees_server` pairs call and cast handling) | Stage 1, A2 | 1.0 |
| gen_statem | Absent | B (`bees_statem`) | A2 | 1.0 |
| gen_event | Absent | B (`bees_event`) | A2 | 1.0 |
| Task, async/await | Absent | B (`bees_task`) | A2 | 1.0 |
| Application: start order, environment, stop | Absent | B (`bees_app`) | A2 | 1.0 |
| Configuration with boot-time validation (`sys.config`) | Absent (`read_lines` works for loading) | B | Stage 1 | 1.0 |
| Releases, release handling | Absent | B | Post-1.0 | Post |
| Behaviour switching (a process replaces its handler) | Spec, roadmap chunk 5 | U (S-10) | Adopted when it lands | Post |

### 1.5 Storage

| Capability | Silica today | Class | Closed by | Target |
| --- | --- | --- | --- | --- |
| ETS `set` / `ordered_set` / `bag` | Absent. `wbt_map` and `wbt_set` exist (persistent, O(log n)) | B (`bees_table`: an actor-owned table) | A2 | 1.0 |
| ETS concurrent reads, `read_concurrency` | Absent: there is no shared mutable memory between actors | Declined for 1.0 (documented difference) | — | — |
| `persistent_term` | Absent | B (boot-time constants) | A2 | 1.0 |
| Hash maps | Absent (`hash<T>` spec-only) | U (S-16) | Adopted when it lands | Post |
| DETS, Mnesia | Absent | Out of scope | — | — |

### 1.6 Naming and registries

| Capability | Silica today | Class | Closed by | Target |
| --- | --- | --- | --- | --- |
| Local registered names (atoms) | Tested (`spawn_registered`, `call_registered`, `cast_registered`) | — | — | — |
| Automatic unregister on death | Unspecified | B | A1 | 1.0 |
| Names that are strings or created at runtime; `via` | Absent (names must be atoms in the compiled image) | B | A1 | 1.0 |
| Global names (`global`), process groups (`pg`) | Absent | B | B4 (compatibility), C1 | 1.0 (BEAM mode) |

### 1.7 Host I/O

| Capability | Silica today | Class | Closed by | Target |
| --- | --- | --- | --- | --- |
| TCP / UDP sockets | Spec §20.4 is a non-normative sketch; nothing implemented | E | Stage 1, A3 | 1.0 |
| Event loop (kqueue / epoll) | Absent | E | A3 | 1.0 |
| Ports with `{active, once}` backpressure | Absent | B over E | A3 | 1.0 |
| DNS A/AAAA | Spec-only (`resolve_hostname`) | E (`getaddrinfo`) | B1 | 1.0 |
| File read | Implemented: `read_lines` returns the whole file, byte-exact | — | — | — |
| File write, directories, file size | Spec-only (only `append_file` and `delete_file` exist) | E now, U later | A3 | 1.0 |
| Byte-level parsing of binary data in Silica | Absent: no `byte_at`, no `bxor`, no bit-casts, no width conversions | U (S-4); E for decoding meanwhile | Stage 1 | 1.0 |

### 1.8 Observability

| Capability | Silica today | Class | Closed by | Target |
| --- | --- | --- | --- | --- |
| Logger with levels, handlers, structured fields | Absent | B | Stage 1, A4 | 1.0 |
| Telemetry events | Absent | B | A4 | 1.0 |
| Tracing (`dbg`-style) | Absent | B hooks, with runtime support as U (S-8) | A4 | Post for full tracing |
| Crash reports | Tested (`FailureReporter`) | — | Wired into logging in A4 | — |

### 1.9 Scheduling and multi-core

| Capability | Silica today | Class | Closed by | Target |
| --- | --- | --- | --- | --- |
| Per-core run queues, work stealing, preemption budget | Spec contradicts itself (§15.1.2.2 vs §26.2.1); no mechanism | U (S-6) | C2 | 1.0 |
| Core pinning, topology, manual migration | Tested (hints only on macOS; `cpu_discovery_and_spawn_pinning` is pending) | — | — | — |
| Placement policy, balancing, overload protection | Absent | B | A4 | 1.0 |
| Watchdog for runaway handlers | Absent (the macOS notes say one is needed) | B | A4 | 1.0 |
| Growable per-actor stacks | Spec, roadmap chunk 1 | U (S-6) | C2 | 1.0 |

### 1.10 Distribution and security

| Capability | Silica today | Class | Closed by | Target |
| --- | --- | --- | --- | --- |
| Node identity, node names | Absent | B | R0.3, B1, B3 | 1.0 |
| **SEMP/TRUST** (TLS 1.3 mTLS, fingerprint whitelist, tokens, endpoint permissions, suspicion) | Absent (the design exists in BEAM_SEMP) | B, with E for TLS | Stage 1, B1, B2 | 1.0 |
| **Standard BEAM distribution** (EPMD, handshake, ETF, control messages) | Absent | B, with E for MD5 and for ETF decoding | Stage 1, B3, B4 | 1.0 |
| External Term Format codec | Absent (§16.3 has no serialization) | B encoder, E decoder until S-4 and S-11 | Stage 1, B3 | 1.0 |
| Remote links, monitors, `nodedown` | Absent | B | I1 | 1.0 |
| Remote spawn, `erpc` | Absent | B (exported endpoints only) | B4 | 1.0 |
| TLS 1.3 engine | Design only (`tls_quantum_safe_future.md`, not on the roadmap) | E now, U (S-13) later | Stage 1 | 1.0 |
| SHA-512, HMAC, MD5, CSPRNG, constant-time compare | Absent (crypto labels are a proposal, roadmap chunk 9) | E now, U (S-14) later | Stage 1 | 1.0 |
| Copy gate for FFI-derived data | The FFI spec forbids direct forwarding; copying is permitted | B (`bees_ingress`) | Stage 1 (S8 spike), B1 | 1.0 |
| TEMPUS (Cyclon, Ed25519, proof of possession) | Absent | B, with E for Ed25519 | Post-1.0 | Post |

### 1.11 Code management and data

| Capability | Silica today | Class | Closed by | Target |
| --- | --- | --- | --- | --- |
| Hot code loading, two-version modules, `code_change` | `hot_swap` effect exists; the mechanism is spec bullets (§26.3.2); dynamic linking is roadmap chunk 8 | U (S-12) plus B for upgrade orchestration | Post-1.0 | Post |
| A universal term type (`term()`) | Absent: no dynamic type, no recursive unions | B (`bees_term` over recursive tuples, spike S6) | R0.3 | 1.0 |
| Atoms created at runtime | Absent (compile-time only) | B: remote atoms are strings; declined as atoms (see §5) | R0.3 | 1.0 |
| Bignums | Spec, roadmap chunk 7 | U (S-11); BEES carries them as opaque bytes meanwhile | B3 | 1.0 as opaque |
| Floats in ETF (`NEW_FLOAT_EXT`) | Needs a bit-cast (absent) | E until S-4 | B3 | 1.0 |
| Bitstrings, bit syntax | Absent | Carried as opaque bytes in BEAM mode; no bit syntax | B3 | 1.0 as opaque |
| Funs on the wire | Absent | Declined (see §5) | — | — |

---

## 2. What BEES provides, by component

| Component | Class | Milestone | Notes |
| --- | --- | --- | --- |
| `bees_pid`, `bees_envelope`, `bees_term`, placement hook | B | R0.3 | The four integration contracts. |
| `bees_node` (boot, configuration, root supervisor) | B | Stage 1 | Fail-fast configuration validation. |
| `bees_timer`, `bees_time` | B over E | Stage 1, A1 | `brodal_okasaki` deadline heap. |
| `bees_signal` (trap-exit, `bees_exit`, remote-aware routing) | B | A1 | Built on Silica link/monitor once S-2 lands. |
| `bees_registry` (string names, auto-unregister, `via`) | B | A1 | Layered on Silica's atom registries. |
| `bees_server`, `bees_statem`, `bees_event`, `bees_task`, `bees_app` | B | A2 | Behaviour traits in the `Supervisor` pattern (spike S1). |
| `bees_table`, `bees_const` | B | A2 | Actor-owned `wbt_map`. |
| `bees_io` (event loop, ports, sockets, files) | B over E | A3 | One loop per core. |
| `bees_log`, `bees_telemetry`, `bees_introspect`, `bees_watchdog`, `bees_place` | B | A4 | |
| `bees_ingress` (the copy gate) | B | Stage 1, B1 | The only path from the native edge into ordinary actors. |
| `bees_trust` (listener, connection FSM, client, whitelist, tokens, suspicion, endpoints) | B over E | Stage 1, B1, B2 | See inter-nodal-modes.md §3. |
| `bees_dist` (EPMD, handshake, control messages, node connections) | B over E | Stage 1, B3, B4 | See inter-nodal-modes.md §4. |
| `bees_etf` (encoder in Silica; decoder token stream from E) | B / E | Stage 1, B3 | |
| `bees_cluster` (membership, partitions, location transparency) | B | C1 | |

---

## 3. Track S — Silica prerequisites

These are work items in the Silica repository. BEES files each as a small proposal with a reproducing trial, under
Silica's `AI_POLICY.md` and fixed-point rules, and may contribute the implementation. "Interim" is what BEES does
until the item lands.

| ID | Item | Why BEES needs it | Blocks | Interim | Evidence |
| --- | --- | --- | --- | --- | --- |
| **S-1** | Atom identity across compilation units | BEES and application modules exchange atoms (`:timeout`, `:down`, `:normal`) constantly. Per-unit atom numbering would make that silently wrong. | Everything, if broken | Spike S2 decides. If broken: only runtime-seeded atoms at BEES API boundaries. | `stdlib/data_structures/brodal_okasaki.silica:25`, `skew_ral_dispatch_bulk.silica:7` versus spec §4.1.7 |
| **S-2** | Runtime `link`, `monitor`, `demonitor` per §15.4.8.4–6 | Lifecycle is the heart of the BEAM surface; a library cannot observe the death of an actor it does not supervise (spike S7). | A1, B3, I1 | Supervisor-only lifecycle in the PoC | `Phase2_TODO/actor_monitor_demonitor_todo.md`; stubs in `prims_actors_runtime_asm.silica` |
| **S-3** | Monotonic clock, sleep, and timer primitive | Every timeout in BEES. | Nothing hard (E stands in) | Native `clock_gettime` and a bounded `poll` | Spec §22.14 `current_time` only; `IPC_Bare.md` "Future Extensions" |
| **S-4** | Byte primitives: byte access to `string`, `string` ↔ `buf(uint8)`, `bxor`, integer width conversions, float bit-cast | Pure-Silica codecs (frames, ETF, SEMP) and retiring the native decoder. | Retiring E decoders | Decoding in the native edge | `utf8_support.md` "Constraint"; `ffi_abi_checker.silica` E2112; spec §8.2.3 |
| **S-5** | Taint copy rule pinned in the spec and trials | BEES's copy gate depends on "copy and send is permitted, direct send is not." That rule needs spec text and a trial so that future taint enforcement keeps the gated path legal. | Nothing, if pinned | `bees_ingress` plus BEES's own trials of the rule | FFI wrapper spec §7.2, §7.6, §7.7; spec open question §16 item 4 |
| **S-6** | Carrier-thread scheduler (per-core run queues, work stealing, dispatch budget, a blocking/dangerous pool) and growable stacks (chunk 1) | BEAM-scale process counts and fairness. | C2 | Thread per actor; PoC scale limits from spike S3 | Spec §15.1.2.2, §23.1.1; `actor_growable_stack_design.md`; `actor_spawn_core_affinity_os_semantics.md` |
| **S-7** | Library consumption: search path, library-rooted `wrapper_meta`, prebuilt library artifacts | Consuming BEES without copying files into every application. | Nothing hard | `tools/bees_config.sh` | Spec `--search-path` unimplemented; FFI wrapper spec §14.1, §14.3 |
| **S-8** | Mailbox introspection (length), optional bounded mailboxes, a tracing hook in dispatch | Overload protection, `process_info`, tracing. | A4 (in part) | Counting at the BEES level for BEES-sent messages only | Spec §16.2.8 (unbounded, no backpressure) |
| **S-9** | Explicit stop, a producer for `(:explicit, atom)`, a resolved call-timeout rule | `exit/2`, orderly shutdown, one consistent call contract. | A1 | Supervisor `:terminate_child`; timeouts through `bees_timer` | Spec §15.1.2.1, §15.4.11.2; §16.1.1 vs §22.4 |
| **S-10** | Variant types and variant patterns (chunk 5) | Heterogeneous messages without wide records; a cleaner `bees_term`. | Nothing hard | Wide inline records and unions | ROADMAP chunk 5; spec §4.2.5, §6.1.2 |
| **S-11** | Big integers (chunk 7) | ETF `SMALL_BIG_EXT` and `LARGE_BIG_EXT` as numbers. | Nothing hard | Opaque bytes | ROADMAP chunk 7 |
| **S-12** | Dynamic linking and `hot_swap` loading (chunk 8) | Code upgrade. | Post-1.0 | None | Spec §26.3.2; ROADMAP chunk 8 |
| **S-13** | Native TLS intrinsics per `tls_quantum_safe_future.md` | Retiring the TLS native edge. **Note:** its `tls_policy` needs a hook exposing the peer certificate or its SHA-512 fingerprint for TRUST whitelisting. | Retiring E | rustls through the native edge (D2) | `tls_quantum_safe_future.md` §5 |
| **S-14** | Crypto labels, `CtMask`, `proc[secret]` (chunk 9) | Constant-time token comparison and key zeroization in Silica. | Retiring E | Constant-time compare in the native edge | `crypto-proposal-introduction.md` |
| **S-15** | Region release for long-lived actors (chunk 4) | Long-lived BEES actors currently cannot allocate without limit. | Nothing hard | Short-lived workers (roadmap §4 item 1) | `region_memory_safety_todo.md` |
| **S-16** | A hash map | O(1) tables and registries. | Nothing hard | `wbt_map` | `atom_actor_registry_direct_index_design.md` §1 |

---

## 4. Native edge inventory

The native edge is a single C archive, `libdangerous_bees_native.a`, reached only through `dangerous_bees_*` Silica
wrapper modules and `spawn_dangerous` workers. Every value it returns enters ordinary actors only through
`bees_ingress`. Payloads cross as `string` (pointer and length); structured results cross as flat records.

| Entry | Purpose | Retires when |
| --- | --- | --- |
| `clock_monotonic_ns`, `clock_system_ns`, `poll_wait(ms)` | Time and the timer tick | S-3 |
| TCP `listen`, `accept`, `connect`, `read`, `write`, `close`; kqueue/epoll registration and wait | Sockets and the event loop | Silica networking intrinsics (none are planned yet) |
| `getaddrinfo` (A/AAAA only) | DNS | Silica networking intrinsics |
| TLS 1.3 session operations (rustls): configure, handshake, read, write, peer certificate DER, SHA-512 of the peer certificate, ALPN | TRUST and TLS distribution | S-13 |
| SHA-512, MD5, HMAC-SHA-512 | Fingerprints, the Erlang cookie challenge | S-4 plus S-14 (SHA-512 and HMAC in Silica); MD5 stays in the edge for as long as BEAM mode exists |
| `random_bytes(n)` (`getentropy` / `arc4random_buf`) | Tokens, request identifiers, handshake challenges | S-14 |
| `ct_equal(a, b)` | Token validation | S-14 |
| ETF decode to a flat token stream (including float bit patterns) | BEAM mode and TRUST payloads | S-4 plus S-11 |
| File write and directory operations | `bees_io` files | Silica `device_io` completion |

**Rules for the edge:**

- No handles are visible to Silica beyond connection identifiers owned by one worker actor.
- No callbacks.
- Every function has a hard size limit on its inputs.
- The archive is rebuilt from pinned sources, and its hash is recorded in the trial tree.

---

## 5. Declined BEAM features

These are not gaps. BEES intentionally does not provide them, and the README's scope section should say so.

| Feature | Reason |
| --- | --- |
| Loading and running `.beam` code; `apply/3` on arbitrary module/function pairs | BEES is not a VM. Remote invocation targets **registered endpoints** only, which is least authority by construction. |
| Creating atoms from remote input | Atoms stay compile-time. Remote atoms are strings, so atom-table exhaustion cannot happen. |
| Selective receive and user-level `receive` | Silica behaviours are invoked once per message by design. BEES offers behaviour-level timeouts and state-machine postponement (`bees_statem`) instead. |
| Process dictionary | Hidden mutable state; the behaviour state record replaces it. |
| Funs and closures on the wire | Remote code execution by construction. |
| Unrestricted remote spawn, `rpc` to anything | Same reason as `apply/3`. BEAM mode exposes exported spawnable endpoints only (B4). |
| Bug-for-bug or complete OTP compatibility | Per the README's scope-out; compatibility is stated per OTP release and per compatibility level. |
