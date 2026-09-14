# BEES Gap Ledger

This ledger maps each BEAM construct that compiled BEAM-language code relies on to what it becomes on Silica. The
target is one of four things:

- a Silica construct;
- a component of the BEES shim;
- an obligation on the language compilers, specified by the BEES target contract;
- the native edge.

For each construct the ledger records Silica's status today and who owns closing the gap. It is the checklist behind
the [roadmap](roadmap.md)'s definition of 1.0, and the seed of the target contract (roadmap R0.3). A row marked
**1.0** is closed only by trials, or by an explicit move out of scope in the README.

**Silica status** (at Apple Silicon fixed point 1, commit `8aff9cdc5`):

- **Tested**: implemented and covered by Silica trials.
- **Stub**: typed by the compiler, but the runtime does nothing.
- **Spec**: specified, not implemented.
- **Absent**: neither specified nor implemented.

**Class** (roadmap §2):

- **U**: upstream; Silica owns it (a Track S item).
- **B**: the BEES shim.
- **C**: an obligation on each language's compiler. Compilers are outside BEES, and the target contract specifies the
  obligation.
- **E**: the native edge.
- **—**: a Silica construct used as-is.

Spec section numbers refer to `silica/compiler/silica-compiler/design_documents/silica-specification.md`. Decisions
D2 and D6–D14 are still open and under discussion ([roadmap §6](roadmap.md#6-decisions)). Where a row cites one of
them, it describes the current proposal.

---

## 1. Construct mapping

### 1.1 Language constructs

| BEAM construct | On Silica | Silica today | Class | Closed by | Target |
| --- | --- | --- | --- | --- | --- |
| Modules, exported and local functions | Silica modules and functions over `bees_term`, using a reserved per-language module prefix | Tested | C (naming in the contract) | T1 | 1.0 |
| Functions with more than 8 parameters | Arguments packed into one tuple, as the contract specifies | Silica allows at most 8 parameters (E3010) | C | T1 | 1.0 |
| `case`, clause patterns, guards | Silica `case` over `bees_term` tags | Tested (`case` is Silica's only branching construct) | C | T1 | 1.0 |
| Tail calls (loops) | Silica tail calls, with an evacuation point and a yield point at each loop back-edge | Tested (recursion is the only loop) | C, B (evacuation and scheduler APIs) | T1, A1, A6 | 1.0 |
| Deep body recursion | Silica stacks | Fixed 512 KB stacks; growable stacks are roadmap chunk 1 | U (S-6) | X2 | 1.0 |
| `receive` with selective matching and `after` | **A plain Silica actor.** The compiler reshapes the code around each `receive` (D1), keeping the paused computation in actor state. BEES supplies the save queue for messages that don't match, and the `after` timers. | User `recv()` is forbidden by design (§15.1.2) | C, B | T1, A2 | 1.0 |
| `try`/`catch`/`throw`/`error`/`exit`, stack traces | Compiler lowering. Result-style values at contract boundaries (D2). | No exceptions (§6.2.3); errors are data | C, B (representation) | T1 | 1.0 |
| Funs (closures), `fun M:F/A` | A fun term (module, index, captured environment), plus a per-module `apply_fun` dispatch entry | Silica closures cannot escape their frame (defect A2, E1067) | C | T1 | 1.0 |
| `apply/2,3`, dynamic `M:F(...)` | Per-module dispatch entries, combined by `bees_config` into the program-wide module table | No dynamic dispatch | C, B (`bees_config`) | T1 | 1.0 |
| Binary construction, bit-syntax matching | The BEES bit-syntax runtime over Silica byte buffers | No byte access, `bxor`, bit-casts or width conversions | U (S-4), B; E for matching until S-4 | A1 | 1.0 |
| Maps, map patterns, map updates | BEES term maps: `wbt_map` ordered by Erlang term order | `wbt_map` tested | B | A1 | 1.0 |

### 1.2 Data

| BEAM construct | On Silica | Silica today | Class | Closed by | Target |
| --- | --- | --- | --- | --- | --- |
| `term()`, i.e. every value | `bees_term`: a recursive tagged tuple (`recursive_tuple_specification.md`) in a region held in actor state | No dynamic type and no recursive unions; recursive tuples exist | B | R0.3, A1 | 1.0 |
| Garbage collection | Evacuation at the compiler's evacuation points, where every root is explicit (roadmap §4.2) | No GC by design; the per-actor arena is never reclaimed | B (API), C (placement); U (S-15) for region release | A1, T1 | 1.0 |
| Small integers | Silica `int64` with checked arithmetic | Tested (`checked_int64_add`, `checked_int64_mul`) | — | Stage 1 | 1.0 |
| Big integers | Silica big integers | Spec (roadmap chunk 7) | U (S-11) | A1 | 1.0 |
| Floats | Silica `float64` | Tested | — | — | — |
| Atoms, including `list_to_atom` | One shared BEES atom table: seeded from every module's manifest, capped at run time. Silica atom literals are used wherever a Silica construct needs an atom. | Silica atoms are compile-time only, and numbered per unit in the implementation | B; U (S-1) for identity across units | Stage 1, A1 | 1.0 |
| Term order, `==` versus `=:=` | BEES comparison over `bees_term` | — | B | A1 | 1.0 |
| Pids, references, ports | A local pid **is** an `actor_ref`. Remote pids, references and ports are BEES terms carrying node and creation. | `actor_ref` is an opaque local handle | B | R0.3, B3 | 1.0 |
| `term_to_binary`, `binary_to_term` | BEES ETF codec: the encoder is in Silica; the decoder is in the native edge until S-4 and S-11 | §16.3 contains no serialization | B, E | A1, B3 | 1.0 |

### 1.3 Processes and signals

| BEAM construct | On Silica | Silica today | Class | Closed by | Target |
| --- | --- | --- | --- | --- | --- |
| Process | **A plain Silica actor** whose messages are `bees_term` (D1) | Tested | — | Stage 1 | 1.0 |
| Lightweight processes in massive numbers | The BEES scheduler (D3), running many plain Silica actors on each carrier thread through the Silica scheduler interface | Spec §15.1.2.2; one pthread per actor today | B; U (S-6) for the interface | A6, X2 | 1.0 |
| `spawn`, `spawn_opt`, spawning on a given core | Silica `spawn`, with the entry function reached through the module table | Tested | — (B for the BIF) | Stage 1 | 1.0 |
| `!` (send) | Silica `cast` of a `bees_term` | Tested | — | Stage 1 | 1.0 |
| Message order per sender/receiver pair | Silica FIFO mailbox | Tested | — | — | — |
| Links, `spawn_link` | Silica `link` | Stub (spec §15.4.8.4–5) | U (S-2) | A2 | 1.0 |
| Monitors, `'DOWN'` | Silica `monitor`/`demonitor`; BEES converts Silica's `(:down, …)` into Erlang's `'DOWN'` term | Stub (spec §15.4.8.6) | U (S-2), B (conversion) | A2 | 1.0 |
| `trap_exit`, `{'EXIT', Pid, Reason}` | Silica trap-exit for ordinary actors, converted into the Erlang term | Absent: only supervisors trap | U (S-9) | A2 | 1.0 |
| `exit/1,2` with term reasons | Silica exit with an opaque payload that carries the term reason | `failure_reason` is a fixed sum, and `(:explicit, atom)` has no producer | U (S-9) | A2 | 1.0 |
| `kill` | Silica `kill_abnormal` | Tested | — | — | — |
| Crash isolation | Silica crash containment and `FailureReporter` | Tested | — | — | — |
| Registered names | The BEES name table, keyed by atom-table atoms; names known at compile time can use Silica's registry directly | Silica registries tested (compile-time atoms only) | B | A2 | 1.0 |
| Timers: `send_after`, `start_timer`, `cancel_timer` | `bees_timer`: a deadline heap (`brodal_okasaki`) over the native clock | Absent | B; E for the clock until S-3 | Stage 1, A2 | 1.0 |
| Process dictionary | A term map kept in actor state, with BEES helpers for it | — | C, B | T1, A2 | 1.0 |
| `process_info`, `processes/0`, `is_process_alive/1` | BEES BIFs over Silica actor introspection | Absent | B; U (S-8) for mailbox data | A2, A3 | 1.0 |
| `hibernate` | An evacuation of the actor's compiler-held state | — | B, C | A4 | 1.0 |
| Group leaders | A process attribute, plus the BEES I/O protocol server | — | B | A5 | 1.0 |

### 1.4 OTP behaviours

| BEAM construct | On Silica | Silica today | Class | Closed by | Target |
| --- | --- | --- | --- | --- | --- |
| `supervisor` | Silica's `Supervisor` trait; each child's start MFA goes through the module table | Tested | — (C for the mapping, B for the start adapter) | Stage 1, A4 | 1.0 |
| `gen_server` (`handle_call`, `handle_cast` and `handle_info` in one process) | Silica's gen_server trait | Partial: a Silica behaviour is either call-only or cast-only (§16.2.6.1) | U (S-19) | A4 | 1.0 |
| `gen_statem` (states, state and event timeouts, postponement) | Silica's state-machine trait | Absent | U (S-17) | A4 | 1.0 |
| `gen_event`, `application`, `proc_lib`, `sys` | OTP's Erlang implementations, compiled by the Erlang compiler, over Silica traits and BEES runtime support | Absent | C (compiled OTP), B | A4, I3 | 1.0 |
| Releases, `code_change` upgrades | — | Absent | U (S-12), B | Post-1.0 | Post |

### 1.5 ERTS modules and BIFs

| BEAM construct | On Silica | Silica today | Class | Closed by | Target |
| --- | --- | --- | --- | --- | --- |
| `erlang` BIFs | BEES BIF library, in coverage tiers | — | B | Stage 1, A3 | 1.0 |
| `ets` | `bees_table`: actor-owned `wbt_map` tables (`set`, `ordered_set`, `bag`, `duplicate_bag`) | Absent | B | A3 | 1.0 |
| `persistent_term`, `atomics`, `counters` | BEES; `atomics` uses Silica's `atomic` memory space where possible | Silica atomics are roadmap chunk 6 | B; U for atomics | A3 | 1.0 |
| `os`, `init`, boot | BEES | Absent | B | A3 | 1.0 |
| `code` | BEES over the program-wide module table; `load_*` and `purge` are unsupported until after 1.0 | Absent | B | A3 | 1.0 |
| `crypto` (hash, HMAC, `strong_rand_bytes`) | BEES over the native edge | Absent (crypto labels are a proposal, chunk 9) | B, E | A3 | 1.0 |
| Pure-Erlang OTP libraries (`lists`, `maps`, `string`, `logger`, …) | Compiled by the Erlang compiler | — | C | I3 | 1.0 |
| `logger` handler back ends | BEES back ends under the compiled `logger` | — | B | A3 | 1.0 |
| Tracing (`erlang:trace`, `dbg`) | BEES trace hooks | Absent | B; U (S-8) for dispatch hooks | A5 (partial) | Post (full) |
| `zlib`, compressed `term_to_binary` | — | Absent | E | Post-1.0 | Post |

### 1.6 Host I/O

| BEAM construct | On Silica | Silica today | Class | Closed by | Target |
| --- | --- | --- | --- | --- | --- |
| `gen_tcp`, `gen_udp`, `inet` | OTP's modules, compiled, over the BEES `prim_inet` layer | Spec §20.4 is a non-normative sketch | C (compiled OTP), B, E | A5, I3 | 1.0 |
| Event loop; ports with `{active, once}` | `bees_io`, one loop per core | Absent | B, E | A5 | 1.0 |
| `file` | OTP's module, compiled, over BEES `prim_file` | `read_lines`, `append_file` and `delete_file` only | C, B, E | A5, I3 | 1.0 |
| Standard I/O and the I/O protocol | BEES I/O server | `print`/`println` | B | Stage 1 subset, A5 | 1.0 |
| DNS | Native `getaddrinfo` | Spec only | E | B1 | 1.0 |
| `ssl` | Not provided in 1.0. TLS belongs to TRUST and BEAM-mode TLS, through the native edge. An `ssl`-compatible subset over the same edge comes after 1.0. | Absent | E, B | Post-1.0 | Post |

### 1.7 Scheduling and observability

| BEAM construct | On Silica | Silica today | Class | Closed by | Target |
| --- | --- | --- | --- | --- | --- |
| Schedulers, reductions, work stealing, priorities | **The BEES scheduler** (`bees_sched`, D3): per-core carrier threads, run queues, work stealing, a dispatch budget standing in for reductions, and a separate pool for blocking and dangerous actors | Spec contradicts itself (§15.1.2.2 vs §26.2.1); no mechanism | B; U (S-6) for the interface | A6 | 1.0 |
| Preemption of long-running code | Yield points at tail calls and receive boundaries, placed by compilers (contract item 6), where the scheduler suspends a process whose budget is spent | None | C, B | T1, A6 | 1.0 |
| Core pinning, topology | Silica | Tested (hints only on macOS) | — | — | — |
| Placement, balancing, overload protection, runaway watchdog | Part of the BEES scheduler | Absent | B | A6 | 1.0 |
| Telemetry | BEES | Absent | B | A5 | 1.0 |

### 1.8 Distribution and security

| BEAM construct | On Silica | Silica today | Class | Closed by | Target |
| --- | --- | --- | --- | --- | --- |
| `node()`, node names, `nodes()` | BEES node identity and distribution BIFs | Absent | B | B1, B3 | 1.0 |
| **SEMP/TRUST** (`trpc:call`, `trpc:cast`) | `bees_trust`, with TLS in the native edge | Absent (the design is in BEAM_SEMP) | B, E | Stage 1, B1, B2 | 1.0 |
| **Standard distribution** (EPMD, handshake, remote send, links, monitors) | `bees_dist` | Absent | B, E | Stage 1, B3 | 1.0 |
| Remote spawn, `erpc`, `rpc` | SPAWN_REQUEST in `bees_dist`; OTP's `erpc` and `rpc` compiled | Absent | B, C | B4, I3 | 1.0 |
| `global`, `pg`, `net_kernel` | OTP's modules, compiled, over BEES distribution BIFs | Absent | C, B | B4, I3 | 1.0 |
| The copy gate for data that came through FFI | `bees_ingress` | The FFI spec forbids forwarding such data directly; copying it is permitted | B | Stage 1, B1 | 1.0 |
| TEMPUS | `bees_tempus`, with Ed25519 in the native edge | Absent | B, E | Post-1.0 | Post |

### 1.9 Code management

| BEAM construct | On Silica | Silica today | Class | Closed by | Target |
| --- | --- | --- | --- | --- | --- |
| Ahead-of-time compilation | Language compiler → Silica compiler → native executable | — | C | I3 | 1.0 |
| Hot code loading, two versions of a module | Silica dynamic linking and `hot_swap` | Spec bullets only (§26.3.2); roadmap chunk 8 | U (S-12), B | Post-1.0 | Post |
| NIFs | Silica FFI wrappers, linked statically | Fifi tested | — (the `dangerous_` naming rule applies) | Post-1.0 (D13) | Post |

---

## 2. BEES components

| Component | Class | Milestone | Role |
| --- | --- | --- | --- |
| Target contract and conformance kit | B | R0.3, T1, T2 | The specification that all language compilers share, plus reference lowerings and a self-check suite. |
| `bees_config` | B (tooling) | R0.1, T1 | Assembles `silica.config`; generates the program-wide module table and the atom-table seed from compiler manifests. |
| `bees_term`, `bees_atom`, `bees_cmp`, `bees_map`, `bees_bits`, `bees_heap` | B | Stage 1, A1 | The term model, atom table, term order, maps, bit syntax, and the evacuation API. |
| `bees_etf` | B, E | A1, B3 | `term_to_binary`/`binary_to_term`, and the wire codec. |
| `bees_recv` (save queue, `after` timers), `bees_pdict`, `bees_signal`, `bees_names`, `bees_timer` | B | Stage 1, A2 | Runtime helpers that reshaped code calls. None of them wraps an actor. |
| `bees_bif_*`, `bees_table`, `bees_pterm`, `bees_atomics`, `bees_os`, `bees_init`, `bees_code`, `bees_crypto` | B, E | A3 | The ERTS-level modules. |
| Behaviour start adapters for Silica's `Supervisor`, gen_server and state-machine traits | B | A4 | The runtime side of contract item 5. |
| `bees_io`, `bees_prim_inet`, `bees_prim_file`, `bees_stdio`, `bees_telemetry` | B, E | A5 | Host I/O and observability. |
| `bees_sched` (carrier threads, run queues, work stealing, dispatch budget, blocking pool, placement, watchdog) | B | A6 | The multi-core scheduler (D3), over the Silica scheduler interface. |
| `bees_ingress` | B | Stage 1, B1 | The copy gate: the only path from the native edge into actors. |
| `bees_trust` (listener, connection FSM, client, whitelist, tokens, suspicion), exposed as `trpc` | B, E | Stage 1, B1, B2 | See inter-nodal-modes.md §3. |
| `bees_dist` (EPMD, handshake, control messages, node connections, distribution BIFs) | B, E | Stage 1, B3, B4 | See inter-nodal-modes.md §4. |

---

## 3. Track S — Silica prerequisites

These are work items in the Silica repository. BEES files each one as a small proposal with a reproducing trial,
under Silica's `AI_POLICY.md` and fixed-point rules, and may contribute the implementation. The **Interim** column
says what BEES does until the item lands. IDs are stable: a withdrawn item keeps its number.

| ID | Item | Why BEES needs it | Blocks | Interim | Evidence |
| --- | --- | --- | --- | --- | --- |
| **S-1** | Atom identity across compilation units | Generated modules, the shim, and Silica constructs exchange atoms constantly. | Everything, if broken | Spike S2 decides | `stdlib/data_structures/brodal_okasaki.silica:25` versus spec §4.1.7 |
| **S-2** | Runtime `link`, `monitor` and `demonitor`, per spec §15.4.8.4–6 | These are the heart of BEAM process semantics, and a library cannot observe the death of an actor it does not supervise (spike S7). | A2, B3, I1 | Supervisor-only lifecycle in the PoC | `Phase2_TODO/actor_monitor_demonitor_todo.md`; the stubs in `prims_actors_runtime_asm.silica` |
| **S-3** | Monotonic clock, sleep, and timer primitives | Every timeout depends on them. | Nothing hard | Native `clock_gettime` and a bounded `poll` | Spec §22.14 |
| **S-4** | Byte primitives: byte access to `string`, conversion between `string` and `buf(uint8)`, `bxor`, integer width conversions, and float bit-casts | Bit syntax, and pure-Silica codecs. | **1.0** (A1) | Matching and decoding in the native edge | `utf8_support.md` "Constraint"; `ffi_abi_checker.silica` E2112; spec §8.2.3 |
| **S-5** | The taint copy rule pinned in spec text and trials | `bees_ingress` relies on the rule that copying and sending is permitted while sending directly is not. | Nothing, if pinned | BEES's own trials of the rule | FFI wrapper spec §7.2, §7.6, §7.7 |
| **S-6** | **A scheduler interface** through which a library runs plain Silica actors. The runtime keeps mailboxes, links, monitors, supervision and crash containment, reports when an actor becomes runnable, and lets the library run one dispatch of that actor on a thread the library owns (D15). Also: **growable stacks** (roadmap chunk 1). | The scheduler lives in BEES (D3), and it must run many actors on each carrier thread. Deep recursion needs stacks that grow during a dispatch. | A6, X2; conformance for deep recursion | One thread per actor; PoC scale taken from spike S3 | Spec §15.1.2.2, §23.1.3; `actor_spawn_core_affinity_os_semantics.md`; `actor_growable_stack_design.md` |
| **S-7** | Library consumption: a search path, `wrapper_meta` paths rooted at the library, and prebuilt library artifacts | Consuming BEES and compiled standard libraries without copying files around. | Nothing hard | `bees_config` | FFI wrapper spec §14.1, §14.3 |
| **S-8** | Mailbox introspection, and a tracing hook in message dispatch | `process_info`, overload protection, and tracing. | A3, A5 (in part) | Counters kept by BEES | Spec §16.2.8 |
| **S-9** | Exit semantics for ordinary actors: trap-exit, `exit/2` sent to another actor, orderly stop, and exit reasons that carry an opaque payload | Erlang exit reasons are arbitrary terms, and trapping processes receive exits as messages. | A2 | None | Spec §15.1.2.1, §15.4.8.5, §15.4.11.2 |
| **S-10** | Variant types (roadmap chunk 5) | A cleaner `bees_term`. | Nothing hard | Recursive tagged tuples | ROADMAP chunk 5 |
| **S-11** | Big integers (roadmap chunk 7) | Erlang integer semantics. | **1.0** (A1) | `int64`, with overflow raising `system_limit` | ROADMAP chunk 7 |
| **S-12** | Dynamic linking and `hot_swap` loading (roadmap chunk 8) | Hot code upgrade. | Post-1.0 | None | Spec §26.3.2 |
| **S-13** | Native TLS intrinsics, with a hook that exposes the peer certificate or its SHA-512 fingerprint | Retiring the TLS native edge; TRUST whitelisting. | Retiring E | rustls through the native edge (D6) | `tls_quantum_safe_future.md` §5 |
| **S-14** | Crypto labels, `CtMask`, `proc[secret]` (roadmap chunk 9) | Constant-time comparison and zeroization inside Silica. | Retiring E | Native constant-time compare | `crypto-proposal-introduction.md` |
| **S-15** | Region release inside a living actor (roadmap chunk 4) | Evacuation, BEES's substitute for GC (roadmap §4.2). | **1.0** (A1) | Short-lived processes only | `region_memory_safety_todo.md` |
| **S-16** | A hash map | O(1) ETS and atom-table operations. | Nothing hard | `wbt_map` | `atom_actor_registry_direct_index_design.md` §1 |
| **S-17** | A state-machine behaviour trait: states, state and event timeouts, postponement | State machines belong in Silica beside the gen_server-style behaviours and the `Supervisor` trait (D5). `gen_statem` modules and the TRUST connection FSM compile to it. | A4; B1 (soft) | The TRUST FSM as a plain behaviour with an explicit phase field | ROADMAP has no chunk for it yet |
| **S-18** | *Withdrawn.* A selective-`receive` primitive for stackful processes. | Not needed: D1 decided that processes are plain actors and compilers reshape code. | — | — | — |
| **S-19** | A gen_server trait in which one process handles `call`, `cast` and raw messages (`info`) | gen_server belongs in Silica (D5), and OTP's `gen_server` puts all three callbacks in one process. | A4 | `handle_info` messages delivered as casts | Spec §16.2.6.1 (a behaviour is either call-only or cast-only) |

---

## 4. Native edge inventory

The native edge is one C archive, `libdangerous_bees_native.a`. Silica code reaches it only through
`dangerous_bees_*` wrapper modules and `spawn_dangerous` workers, and every value it returns reaches actors only
through `bees_ingress`. Payloads cross the boundary as `string` values (pointer and length); structured results cross
as flat records.

| Entry | Purpose | Retires when |
| --- | --- | --- |
| `clock_monotonic_ns`, `clock_system_ns`, `poll_wait(ms)` | Time and the timer tick | S-3 |
| TCP and UDP socket calls; kqueue/epoll registration and wait | `bees_io`, `prim_inet` | Silica networking intrinsics (none are planned yet) |
| `getaddrinfo` (A/AAAA records only) | DNS | Silica networking intrinsics |
| File write, directory operations, file metadata | `prim_file` | Silica `device_io` completion |
| TLS 1.3 session operations via rustls: configure, handshake, read, write, peer certificate DER and its SHA-512, ALPN | TRUST, BEAM-mode TLS | S-13 |
| SHA-2 family, MD5, HMAC | `crypto`, certificate fingerprints, the Erlang cookie challenge | S-4 plus S-14 (MD5 stays for as long as BEAM mode exists) |
| `random_bytes(n)` | `crypto:strong_rand_bytes`, tokens, challenges | S-14 |
| `ct_equal(a, b)` | Token validation | S-14 |
| Binary decoding helpers: the ETF token stream, bit-syntax field extraction, float bit patterns | `binary_to_term`, bit-syntax matching | S-4 plus S-11 |

**Rules for the edge:**

- No handles are visible to Silica beyond connection identifiers owned by one worker actor.
- No callbacks.
- Every entry point enforces a hard limit on the size of its inputs.
- The archive is rebuilt from pinned sources, and its hash is recorded in the trial tree.

---

## 5. Not supported

Compiling BEAM languages to Silica keeps nearly all of the BEAM's *semantics*. What it gives up are the BEAM
*mechanisms* that assume an interpreter.

| Feature | Status | Reason |
| --- | --- | --- |
| Loading or interpreting `.beam` bytecode at run time | Not supported | Code is compiled ahead of time by the language compilers. |
| Calling a local fun received from an Erlang node in BEAM mode | Not supported. The fun is carried as an opaque term, and calling it raises `badfun`. | Its code is BEAM bytecode. Export funs (`fun M:F/A`) work through the module table. |
| Hot code loading | Post-1.0 (S-12) | Needs Silica dynamic linking. |
| NIFs | Post-1.0 (D13) | They are written against the BEAM's C API. |
| ETS concurrency guarantees | The options are accepted, and the actual semantics are documented | Silica actors share no mutable memory, so every table access is serialized through the table's owner. |
| Emulator introspection (`erts_debug`, scheduler `system_flag`s, GC statistics) | Accepted as no-ops or mapped (`garbage_collect` triggers an evacuation), and documented | These describe emulator internals that do not exist here. |
| Bug-for-bug OTP compatibility | Not claimed | As the README states; semantics follow one named OTP release (D14). |
