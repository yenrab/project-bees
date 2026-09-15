# BEES Gap Ledger

This ledger maps each BEAM construct that compiled BEAM-language code relies on to what it becomes on Silica. The
target is one of four things:

- a Silica construct;
- a component of the BEES shim;
- an obligation on the language compilers, specified by the BEES target contract;
- the native edge.

BEES makes compiling a BEAM language to Silica **possible, not easy** (roadmap D19). So a row's goal is a *route*
to Silica, not a reproduction of BEAM behaviour. Where Silica can already express something, however awkwardly,
the row goes to the compilers (class C). What BEES deliberately does not provide is listed in §5.

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
Every decision it cites has been made ([roadmap §6](roadmap.md#6-decisions)).

---

## 1. Construct mapping

### 1.1 Language constructs

| BEAM construct | On Silica | Silica today | Class | Closed by | Target |
| --- | --- | --- | --- | --- | --- |
| Modules, exported and local functions | Silica modules and functions over `bees_term`, using a reserved per-language module prefix | Tested | C (naming in the contract) | T1 | 1.0 |
| Functions with more than 8 parameters | Arguments packed into one tuple, as the contract specifies | Silica allows at most 8 parameters (E3010) | C | T1 | 1.0 |
| `case`, clause patterns, guards | Silica `case` over `bees_term` tags | Tested (`case` is Silica's only branching construct) | C | T1 | 1.0 |
| Tail calls (loops) | Silica tail calls, with an evacuation point and a yield point at each loop back-edge | Tested (recursion is the only loop) | C, B (evacuation and yield helpers) | T1, A1, A6 | 1.0 |
| Deep body recursion | Silica stacks | Fixed 512 KB stacks; growable stacks are roadmap chunk 1 | U (S-6) | X2 | 1.0 |
| `receive` with selective matching and `after` | **A plain Silica actor.** The compiler reshapes the code around each `receive` (D1), keeping the paused computation in actor state. BEES supplies the save queue for messages that don't match, and the `after` timers. | User `recv()` is forbidden by design (§15.1.2) | C, B | T1, A2 | 1.0 |
| `try`/`catch`/`throw`/`error`/`exit`, stack traces | **Nothing is caught inside the actor** (D2). A failing BIF, `badmatch`, `badarith`, `throw` and similar all fail the actor, and its supervisor reports the exit (D23). **Compilers reject `catch` clauses, `after` clauses and `catch Expr` at compile time.** BIFs never return failures as values. BEES provides result-returning versions of commonly caught BIFs, including a call variant that returns `timeout` as a value; a compiler may rewrite a `try` that catches exactly one of those failures into a call to the variant. Stack traces come from Silica's crash report. | No exceptions (§6.2.3); errors are data; a fault or `panic` ends the actor | C (compile-time rejection and rewrites); B (the BIF variants); U (S-9) for an explicit abnormal stop | T1, A2, A3 | 1.0 |
| Funs (closures), `fun M:F/A` | A fun term (module, index, captured environment), plus a per-module `apply_fun` dispatch entry | Silica closures cannot escape their frame (defect A2, E1067) | C | T1 | 1.0 |
| `apply/2,3`, dynamic `M:F(...)` | Per-module dispatch entries, combined by `bees_config` into the program-wide module table | No dynamic dispatch | C, B (`bees_config`) | T1 | 1.0 |
| Binary construction, bit-syntax matching | Compiler lowering over binaries held as `buf(uint8)` plus a bit length. Literals become bytes at compile time; run-time values use lookups the compiler generates and same-type arithmetic (roadmap §4.5). | No type conversion, by design (§8.2.3); `buf(uint8)` exists | C | T1 | 1.0 |
| Integer bitwise operators (`band`, `bor`, `bxor`, `bnot`, `bsl`, `bsr`) | Compiler lowering: an `int64` crosses to `uint64` byte by byte through lookups the compiler generates, Silica's `uint64` operators apply, and the result crosses back. `bxor` is `(a bor b) band bnot (a band b)`. | `bor`, `band`, `bnot`, `shl` and `shr` on `uint64` only (§7.3.1); no `bxor` | C; U (S-11) beyond `int64` | T1 | 1.0 |
| Maps, map patterns, map updates | BEES term maps: `wbt_map` ordered by Erlang term order | `wbt_map` tested | B | A1 | 1.0 |

### 1.2 Data

| BEAM construct | On Silica | Silica today | Class | Closed by | Target |
| --- | --- | --- | --- | --- | --- |
| `term()`, i.e. every value | `bees_term`: a recursive tagged tuple (`recursive_tuple_specification.md`) in a region held in actor state | No dynamic type and no recursive unions; recursive tuples exist | B | R0.3, A1 | 1.0 |
| Garbage collection | Evacuation at the compiler's evacuation points, where every root is explicit (roadmap §4.2) | No GC by design; the per-actor arena is never reclaimed | B (API), C (placement); U (S-15) for region release | A1, T1 | 1.0 |
| Small integers | Silica `int64` with checked arithmetic | Tested (`checked_int64_add`, `checked_int64_mul`) | — | Stage 1 | 1.0 |
| Big integers | Silica big integers | Spec (roadmap chunk 7) | U (S-11) | A1 | 1.0 |
| Floats | Silica `float64` | Tested | — | — | — |
| Atoms | **Silica atoms**, in Silica's atom table. Compilers emit every atom as a Silica atom literal. BEES has no atom table of its own (roadmap §4.6). | Tested as compile-time constants. The spec's one global table (§4.1.7) is numbered per unit in the implementation. | — ; U (S-1) for identity across units | Stage 1 | 1.0 |
| `atom_to_list`, `list_to_existing_atom`, `list_to_atom`, and their binary forms | **Lookups, not conversions.** BEES BIFs over the program-wide atom lookup that `bees_config` generates from compiler manifests, pairing each atom literal with its spelling as a string literal (roadmap §4.6). No atom is created at run time: `list_to_atom` returns an atom only if the lookup holds it (D16). | Silica has no type conversion, and BEES adds none | B; C (manifests list atoms) | T1, A1 | 1.0 |
| Term order, `==` versus `=:=` | BEES comparison over `bees_term` | — | B | A1 | 1.0 |
| Pids, references, ports | A local pid is an `actor_ref`, identified by its **supervision-tree address** (D22). A remote pid is the node plus that address. References and ports are BEES terms carrying their node. | `actor_ref` is a pointer to a control block that is freed at teardown; the spec defines no identity, equality or order | U (S-26); B for remote pids | R0.3, A2, B3 | 1.0 |
| `term_to_binary`, `binary_to_term` | BEES ETF codec, encoder and decoder both in Silica, using BEES's own lookups and same-type arithmetic (roadmap §4.5); big integers wait on S-11 | §16.3 contains no serialization | B | A1, B3 | 1.0 |

### 1.3 Processes and signals

| BEAM construct | On Silica | Silica today | Class | Closed by | Target |
| --- | --- | --- | --- | --- | --- |
| Process | **A plain Silica actor** whose messages are `bees_term` (D1) | Tested | — | Stage 1 | 1.0 |
| Lightweight processes in massive numbers | Silica's per-core scheduler, running many plain Silica actors on each core's carrier thread (spec §15.1.2, Actor Pinning Policy; §23.1.1). BEES balances them across cores (D3). | Specified; one pthread per actor today | U (S-6); B for balancing | A6, X2 | 1.0 |
| `spawn`, `spawn_opt`, spawning on a given core | Silica `spawn`, with the entry function reached through the module table. **Every actor has a supervisor** (D18): a plain `spawn` adds a `temporary` child to a BEES supervisor, and those supervisors are sharded per core. Before one runs out of child numbers, it is replaced by a supervisor at a fresh position (S-26). | Tested | — (B for the BIF and the supervisors) | Stage 1, A2 | 1.0 |
| `!` (send) | Silica `cast` of a `bees_term` | Tested | — | Stage 1 | 1.0 |
| Call timeouts (`gen_server:call(S, M, Timeout)`) | A timeout fails the caller (D2). **A call variant returns `timeout` as a value instead**, for code that expects to handle it. The timer comes from `bees_timer`. | The spec contradicts itself: §16.1.1 has no timeout, §22.4 a fixed 5-second one | B; U (S-9) for a resolved call-timeout rule | A2, A3 | 1.0 |
| Message order per sender/receiver pair | Silica FIFO mailbox | Tested | — | — | — |
| Links, `spawn_link` | **The compiler's choice** (D18). It can use Silica `link` as it is, where a normal exit also ends the peer, or it can build link-like behaviour from exit requests to supervisors. | Stub (spec §15.4.8.4–5) | C; U (S-2) | T1, A2 | 1.0 |
| Monitors (`erlang:monitor/2`, `spawn_monitor`, `monitor_node`) | **Not provided** (D20, §5). A pending Silica `call` already wakes with the callee's death result, and supervisors handle fate-sharing. | Specified; a stub in the runtime (§15.4.8.6) | — | — | — |
| `trap_exit`, `{'EXIT', Pid, Reason}` | **Not provided** (D18, §5). In Silica only supervisors trap exits. Exits are handled by supervisors. | Absent by design | — | — | — |
| `exit/1,2` | **A request to the target's supervisor** (D18). The pid's supervisor position (D22) identifies the supervisor, through a key-value collection the compiler builds from a Silica collection (for example `wbt_map`); the supervisor shuts the target down and reports it. Reasons are Silica `failure_reason` atoms. A process ends itself through `remove_actor(self())` or the same request (S-9). A restarted child has a new address (D22), so a request aimed at an old pid never reaches its replacement. | `call_supervisor` `:terminate_child` tested; no lookup of an actor's supervisor, and `actor_ref` equality unspecified | C; U (S-26, S-9) | T1, A2 | 1.0 |
| `kill` | Silica `kill_abnormal`, or the supervisor's brutal shutdown (`shutdown: 0`) | Named in the spec, without a signature | — ; U (S-9) | A2 | 1.0 |
| Crash isolation | Silica crash containment and `FailureReporter` | Tested | — | — | — |
| Registered names | The BEES name table (`register/2`, `unregister/1`, `whereis/1`), keyed by Silica atoms. The exit hub removes a dead pid's names (D23); a process registered as it is spawned can use Silica's registry directly | Silica registries tested (registration at spawn). In the implementation, a supervised child is registered under its child-spec `id` and re-registered under the same `id` when it is restarted, so the name reaches the new actor; the spec does not say so, and no trial looks a name up after a restart. A name registered with plain `spawn_registered` is never removed, so after the actor dies it points at a freed control block. | B | A2 | 1.0 |
| Timers: `send_after`, `start_timer`, `cancel_timer` | `bees_timer`: a deadline heap (`brodal_okasaki`) over the native clock | Absent | B; E for the clock until S-3 | Stage 1, A2 | 1.0 |
| Process dictionary | A term map kept in actor state, with BEES helpers for it | — | C, B | T1, A2 | 1.0 |
| `process_info`, `processes/0`, `is_process_alive/1` | BEES BIFs over Silica actor introspection | Absent | B; U (S-8) for mailbox data | A2, A3 | 1.0 |
| `hibernate` | An evacuation of the actor's compiler-held state | — | B, C | A4 | 1.0 |
| Group leaders | A process attribute, plus the BEES I/O protocol server | — | B | A5 | 1.0 |

### 1.4 OTP behaviours

| BEAM construct | On Silica | Silica today | Class | Closed by | Target |
| --- | --- | --- | --- | --- | --- |
| `supervisor` | Silica's `Supervisor` trait; each child's start MFA goes through the module table. Every actor has a supervisor (D18). | Tested | — (C for the mapping, B for the start adapter) | Stage 1, A4 | 1.0 |
| `gen_server` (`handle_call`, `handle_cast` and `handle_info`) | **Two Silica gen_server-style actors**, one call-only and one cast-only (D5). The contract fixes which actor holds the state, where `handle_info` messages go, and how a client's cast followed by a call stays in order. | Tested: a Silica behaviour is either call-only or cast-only (§16.2.6.1) | — (C for the split) | T1, A4 | 1.0 |
| `gen_statem` (states, state and event timeouts, postponement) | Silica's state-machine trait | Absent | U (S-17) | A4 | 1.0 |
| `gen_event`, `application`, `proc_lib`, `sys` | OTP's Erlang implementations, compiled by the Erlang compiler, over Silica traits and BEES runtime support | Absent | C (compiled OTP), B | A4, I3 | 1.0 |
| Releases, `code_change` upgrades | — | Absent | U (S-12), B | Post-1.0 | Post |

### 1.5 ERTS modules and BIFs

| BEAM construct | On Silica | Silica today | Class | Closed by | Target |
| --- | --- | --- | --- | --- | --- |
| `erlang` BIFs | BEES BIF library, in coverage tiers. A failing BIF fails the actor (D2). Commonly caught BIFs also get result-returning versions, for example `{ok, Atom} \| error`. | — | B | Stage 1, A3 | 1.0 |
| `ets` | `bees_table`: actor-owned `wbt_map` tables (`set`, `ordered_set`, `bag`, `duplicate_bag`). The exit hub deletes a table when the process that owns it exits (D23). | Absent | B | A3 | 1.0 |
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
| `gen_tcp`, `gen_udp`, `inet` | OTP's modules, compiled, over the BEES `prim_inet` layer, which sits on Silica's own TCP/IP implementation | Planned by the Silica project; spec §20.4 is a non-normative sketch | C (compiled OTP), B; U (S-22); E until S-22 | A5, I3 | 1.0 |
| Event loop; ports with `{active, once}` | `bees_io`, one loop per core | Absent | B; E until S-22 | A5 | 1.0 |
| `file` | OTP's module, compiled, over BEES `prim_file` | `read_lines`, `append_file` and `delete_file` only | C, B, E | A5, I3 | 1.0 |
| Standard I/O and the I/O protocol | BEES I/O server | `print`/`println` | B | Stage 1 subset, A5 | 1.0 |
| DNS | Silica's `resolve_hostname` (S-22); until then, native `getaddrinfo`, whose results need re-creation (S-5) | Spec only | U (S-22); E until then | B1 | 1.0 |
| `ssl` | Not provided in 1.0. TLS belongs to TRUST and BEAM-mode TLS, through the native edge. An `ssl`-compatible subset over the same edge comes after 1.0. | Absent | E, B | Post-1.0 | Post |

### 1.7 Scheduling and observability

| BEAM construct | On Silica | Silica today | Class | Closed by | Target |
| --- | --- | --- | --- | --- | --- |
| Schedulers, run queues, fairness, priorities | **Silica's per-core scheduler** (spec §23.1.1). Silica never interrupts a dispatch (§15.1.2), so BEES adds a dispatch-budget helper that compiled code checks at yield points. | Specified, but the spec contradicts itself on preemption (§15.1.2 vs §23.1.1); not implemented | U (S-6); C, B for yield points | A6 | 1.0 |
| Preemption of long-running code | Yield points at tail calls and receive boundaries, placed by compilers (contract item 6). A process whose budget is spent ends its dispatch there and continues through `cast(self())`. | None | C, B | T1, A6 | 1.0 |
| Core pinning, topology | Silica's Actor Pinning Policy (§15.1.2): every actor is pinned from spawn until it terminates. Running raw, a pin is exclusive and hard. OS-hosted, the actor is bound hard to its core's carrier thread, but the OS may move that thread, so exclusivity is never guaranteed (roadmap §4.10). | Specified; tested as hints only on macOS | — | — | — |
| Placement, balancing, overload protection, runaway watchdog | **BEES** (D3), entirely through `migrate_actor()` and the core id given at spawn. OS-hosted, it balances BEAM-style between the runtime's carrier threads; running raw, directly onto the cores. It keeps cores reserved for blocking and dangerous actors. Each migration takes effect at a dispatch boundary, and explicitly placed processes are never moved. | Specified: the runtime never moves an actor, and programs migrate with `migrate_actor()` (§15.1.2) | B; U (S-23) for `migrate_actor()` on the raw targets | A6 | 1.0 |
| Telemetry | BEES | Absent | B | A5 | 1.0 |

### 1.8 Distribution and security

| BEAM construct | On Silica | Silica today | Class | Closed by | Target |
| --- | --- | --- | --- | --- | --- |
| `node()`, node names, `nodes()` | BEES node identity and distribution BIFs | Absent | B | B1, B3 | 1.0 |
| **SEMP/TRUST** (`trpc:call`, `trpc:cast`) | `bees_trust`, with TLS in the native edge | Absent (the design is in BEAM_SEMP) | B, E | Stage 1, B1, B2 | 1.0 |
| **Standard distribution** (EPMD, handshake, remote send, links) | `bees_dist` | Absent | B, E | Stage 1, B3 | 1.0 |
| Remote spawn, `erpc`, `rpc` | SPAWN_REQUEST in `bees_dist`; OTP's `erpc` and `rpc` compiled | Absent | B, C | B4, I3 | 1.0 |
| `global`, `pg`, `net_kernel` | OTP's modules, compiled, over BEES distribution BIFs | Absent | C, B | B4, I3 | 1.0 |
| Bringing FFI-derived data into BEES | **Re-creation** (S-5), a compiler-implemented trait, followed by `bees_ingress` protocol validation | The FFI spec keeps FFI-derived data tainted however it is copied (§7.4). De-taint by validation is reserved for a future spec version (§7.7). | U (S-5), B (`bees_ingress`) | Stage 1, B1 | 1.0 |
| TEMPUS | `bees_tempus`, with Ed25519 in the native edge | Absent | B, E | Post-1.0 | Post |

### 1.9 Code management

| BEAM construct | On Silica | Silica today | Class | Closed by | Target |
| --- | --- | --- | --- | --- | --- |
| Ahead-of-time compilation | Language compiler → Silica compiler → native executable | — | C | I3 | 1.0 |
| Hot code loading, two versions of a module | Silica dynamic linking and `hot_swap` | Spec bullets only (§26.3.2); roadmap chunk 8 | U (S-12), B | Post-1.0 | Post |
| NIFs | **Reworked by the language compiler** into a Silica actor that wraps the external call through Fifi: a `dangerous_*` module and a `spawn_dangerous` worker, as standard Silica does (D13). `erlang:load_nif` is not provided. | Fifi tested | C; U (S-5) for results used beyond the receiving handler; U (S-24) on the raw targets | T1 | 1.0 |

---

## 2. BEES components

| Component | Class | Milestone | Role |
| --- | --- | --- | --- |
| Target contract and conformance kit | B | R0.3, T1, T2 | The specification that all language compilers share, plus reference lowerings and a self-check suite. |
| `bees_config` | B (tooling) | R0.1, T1 | Assembles `silica.config`; generates the program-wide module table and atom lookup from compiler manifests. |
| `bees_term`, `bees_cmp`, `bees_map`, `bees_bits`, `bees_heap` | B | Stage 1, A1 | The term model (atoms are Silica atoms), term order, maps, bit syntax, and the evacuation API. |
| `bees_etf` | B, E | A1, B3 | `term_to_binary`/`binary_to_term`, and the BEAM-mode wire codec. TRUST has its own encoding (D8). |
| `bees_recv` (save queue, `after` timers), `bees_pdict`, `bees_signal`, `bees_names`, `bees_timer`, `bees_exit_hub` (per core) | B | Stage 1, A2 | Runtime helpers that reshaped code calls, and the exit hub that receives supervisor exit reports (D23). None of them wraps an actor. |
| `bees_bif_*`, `bees_table`, `bees_pterm`, `bees_atomics`, `bees_os`, `bees_init`, `bees_code`, `bees_crypto` | B, E | A3 | The ERTS-level modules. |
| Behaviour adapters: Silica's `Supervisor` trait, the call/cast gen_server split, and the state-machine trait | B | A4 | The runtime side of contract item 5. |
| `bees_io`, `bees_prim_inet`, `bees_prim_file`, `bees_stdio`, `bees_telemetry` | B, E | A5 | Host I/O and observability. |
| `bees_place` (balancer, placement hook, reserved cores for blocking and dangerous actors, dispatch-budget helper, watchdog, load statistics) | B | A6 | Balancing and placement across cores through `migrate_actor()` (D3). |
| `bees_ingress` | B | Stage 1, B1 | Protocol validation for all network input: TLS plaintext after re-creation, and bytes from Silica's own sockets. |
| `bees_trust` (listener, connection FSM, client, whitelist, tokens, suspicion, the `trust/1` term codec), exposed as `trpc` | B, E | Stage 1, B1, B2 | See inter-nodal-modes.md §3. |
| `bees_dist` (EPMD, handshake, control messages, node connections, distribution BIFs) | B, E | Stage 1, B3, B4 | See inter-nodal-modes.md §4. |

---

## 3. Track S — Silica prerequisites

These are work items in the Silica repository. BEES files each one as a small proposal with a reproducing trial,
under Silica's `AI_POLICY.md` and fixed-point rules, and may contribute the implementation. The **Interim** column
says what BEES does until the item lands. IDs are stable: a withdrawn item keeps its number.

| ID | Item | Why BEES needs it | Blocks | Interim | Evidence |
| --- | --- | --- | --- | --- | --- |
| **S-1** | Atom identity across compilation units: the one global atom table the spec promises | BEAM atoms are Silica atoms, and BEES has no atom table of its own. Generated modules, the shim, and Silica constructs exchange atoms constantly. | Everything, if broken | Spike S2 decides | `stdlib/data_structures/brodal_okasaki.silica:25` versus spec §4.1.7 |
| **S-2** | Runtime `link`, per spec §15.4.8.4–5. BEES does not need Silica's `monitor` or `demonitor` (D20). | Compilers may use Silica links for BEAM links (D18), and a library cannot observe deaths it does not supervise (spike S7). | A2, B3, I1 | Supervisor-only lifecycle in the PoC | `Phase2_TODO/actor_monitor_demonitor_todo.md`; the stubs in `prims_actors_runtime_asm.silica` |
| **S-3** | Monotonic clock, sleep, and timer primitives | Every timeout depends on them. | Nothing hard | Native `clock_gettime` and a bounded `poll` | Spec §22.14 |
| **S-4** | *Withdrawn.* Byte primitives: byte access to `string`, conversion between `string` and `buf(uint8)`, `bxor`, integer width conversions, and float bit-casts. | Not needed. Silica has no type conversion and BEES adds none: compilers lower bit syntax and bitwise operators with lookups they generate, BEES's codecs do the same, and binaries are never Silica strings (roadmap §4.5). What remains is S-21 and S-22. | — | — | — |
| **S-5** | **Re-creation:** a compiler-implemented trait (working name `Recreatable`), the future validator version reserved by FFI spec §7.7. It is established on an inline type, with limits only. The compiler derives `recreate(x) -> (:ok, T) \| (:rejected, atom)`, which checks the declared limits before reading contents, rebuilds the value in a fresh region the caller owns, rebuilds enumerations from the type's own literals, and range-checks everything else. It may be called only where tainted data may legally be. Its output is pure, but still barred from `hot_swap` and command execution (§7.2). | TLS and secure random bytes come through the native edge (D17). Without re-creation, TLS plaintext and ciphertext, random bytes, digests and peer-certificate data cannot leave the handler that receives them (§7.3, §7.6). | B1, B3, every native-edge flow | None: FFI output is unusable outside its handler. The protocol layers run over an in-process loopback transport. | FFI spec §7.3, §7.4, §7.6, §7.7; open questions 4 and 5 in §16 |
| **S-6** | **Silica's per-core scheduler, as its spec describes** (Actor Pinning Policy §15.1.2, §23.1.1): a carrier thread per logical core running many actors, with fairness, preemption and priority. The spec should also define "scheduler yield point" and reconcile §15.1.2 (nothing interrupts a dispatch) with §23.1.1 (long-running processes can be preempted). Also: **growable stacks** (roadmap chunk 1). | Per-core scheduling is Silica's (D3). BEAM process counts need many actors per carrier thread, and deep recursion needs stacks that grow during a dispatch. | A6 (scale), X2; conformance for deep recursion | One thread per actor; PoC scale taken from spike S3 | Spec §15.1.2, §23.1.1; `actor_growable_stack_design.md`; `Phase1_TODOs/actor_stack_growth_plan.md` |
| **S-7** | Library consumption: a search path, `wrapper_meta` paths rooted at the library, and prebuilt library artifacts | Consuming BEES and compiled standard libraries without copying files around. | Nothing hard | `bees_config` | FFI wrapper spec §14.1, §14.3 |
| **S-8** | Mailbox introspection, and a tracing hook in message dispatch | `process_info`, overload protection, and tracing. | A3, A5 (in part) | Counters kept by BEES | Spec §16.2.8 |
| **S-9** | **Stopping and shutdown** for ordinary actors:<br>• a way for an actor to stop itself, either `remove_actor(self())` stated as legal or a stop return form for behaviours;<br>• a defined graceful-shutdown signal from a supervisor to its child, which the spec says is sent but never defines;<br>• removal of `temporary` children from the child table when they exit;<br>• a signature and a distinguishable reason for `kill_abnormal`;<br>• one call-timeout rule, since §16.1.1 has none and §22.4 has a fixed 5 seconds, with a per-call timeout. | Every actor has a supervisor, and exits go through supervisors (D18). Without a self-stop, a process asking its own supervisor to end it waits out its shutdown timeout. Without the signal, no cleanup code can run. Without removal, the table grows with every `spawn`. | A2 (soft) | `shutdown: 0` (brutal) for BEAM processes; cleanup such as `terminate/2` does not run | Spec §15.1.2.1, §15.4.12, §15.4.13.3, §22.10 |
| **S-10** | Variant types (roadmap chunk 5) | A cleaner `bees_term`. | Nothing hard | Recursive tagged tuples | ROADMAP chunk 5 |
| **S-11** | Big integers (roadmap chunk 7) | Erlang integer semantics. | **1.0** (A1) | `int64`, with overflow raising `system_limit` | ROADMAP chunk 7 |
| **S-12** | Dynamic linking and `hot_swap` loading (roadmap chunk 8) | Hot code upgrade. | Post-1.0 | None | Spec §26.3.2 |
| **S-13** | Native TLS intrinsics, with a hook that exposes the peer certificate or its SHA-512 fingerprint | Retiring the TLS native edge; TRUST whitelisting. | Retiring E | rustls through the native edge (D6) | `tls_quantum_safe_future.md` §5 |
| **S-14** | Crypto labels, `CtMask`, `proc[secret]` (roadmap chunk 9) | Constant-time comparison and zeroization inside Silica. | Retiring E | Native constant-time compare | `crypto-proposal-introduction.md` |
| **S-15** | Region release inside a living actor (roadmap chunk 4) | Evacuation, BEES's substitute for GC (roadmap §4.2). | **1.0** (A1) | Short-lived processes only | `region_memory_safety_todo.md` |
| **S-16** | A hash map | O(1) ETS operations. | Nothing hard | `wbt_map` | `atom_actor_registry_direct_index_design.md` §1 |
| **S-17** | A state-machine behaviour trait: states, state and event timeouts, postponement | State machines belong in Silica beside the gen_server-style behaviours and the `Supervisor` trait (D5). `gen_statem` modules and the TRUST connection FSM compile to it. | A4; B1 (soft) | The TRUST FSM as a plain behaviour with an explicit phase field | ROADMAP has no chunk for it yet |
| **S-18** | *Withdrawn.* A selective-`receive` primitive for stackful processes. | Not needed: D1 decided that processes are plain actors and compilers reshape code. | — | — | — |
| **S-19** | *Withdrawn 2026-09-15.* A gen_server trait in which one process handles `call`, `cast` and raw messages. | Not needed: an Erlang `gen_server` that uses both becomes two Silica gen_server-style actors, one call-only and one cast-only (D5). | — | — | — |
| **S-20** | *Withdrawn.* Spelling access to Silica's atom table. | Not needed: Silica has no type conversion and BEES adds none, so atom spellings come from the generated atom lookup (roadmap §4.6). | — | — | — |
| **S-21** | `buf(region, uint8)` across the FFI boundary, as the FFI wrapper spec describes | Whatever still passes through the native edge (TLS records until S-13, file data) must cross as bytes, because binaries are `buf(uint8)` and never Silica strings (roadmap §4.5). | Nothing hard | The edge packs bytes into records of `uint64` words, which BEES unpacks with `shr`, `band` and lookups | FFI wrapper spec §6.2, §6.4 and its type table; `ffi_abi_checker.silica` E2112 (scalars and inline records only at FP1) |
| **S-22** | Silica's own TCP/IP implementation | Sockets for `prim_inet`, TRUST and BEAM mode, with network bytes delivered as `buf(uint8)`. It retires the native edge's sockets and event-loop entries. | Nothing hard | Native-edge sockets with kqueue/epoll | Planned by the Silica project, not yet in its ROADMAP; spec §20.4 sketches socket operations over `buf(uint8)`; until it lands on the raw targets, sockets there go through Fifi (S-24) |
| **S-23** | `migrate_actor()` and exclusive pins **implemented** on the raw targets (ESP32-S3 and AArch64) | Running raw, BEES balances directly onto the cores, and `migrate_actor()` is the only way anything changes core (roadmap §4.10). | A6 (raw) | None; running raw, processes stay where they were spawned | Now specified: the Actor Pinning Policy (§15.1.2) makes pins exclusive on OS-free targets, lets only the program move an actor, applies a move at the next dispatch boundary or yield point, and preserves message order. What remains is the implementation; the ESP32-S3 port runs a cooperative scheduler on one core (`esp32s3_port_status.md`). |
| **S-24** | Fifi on the raw targets (ESP32-S3 and AArch64) | To begin with, the raw targets reach the native edge's facilities (sockets, TLS, hashes, random bytes, the clock) through Fifi, with the edge built against each board instead of an OS (roadmap §5, Release 1.0 item 8). | The native edge on the raw targets | None | `esp32s3_port_status.md`: "foreign (C) calls are not supported on this target" (no C runtime; the hosted guarded FFI is setjmp and signal based); `porting_for_os_free_targets.md` lists Fifi as optional for OS-free runtimes; Silica ROADMAP: ESP32-S3 FP1 replaces "Fifi against OS libraries" with board-pack equivalents |
| **S-25** | *Withdrawn 2026-09-15.* Runtime notifications to an actor whose message type is something else. | Not needed by BEES: monitors are not part of BEES (D20). It remains an issue inside Silica: an actor that calls `monitor/1` is killed by a `DOWN` its message type does not admit (§16.2.3). | — | — | — |
| **S-26** | **Actor identity by supervision-tree address** (D22), with `actor_ref` equality and a total order defined on it.<br>• An actor's identity is its supervisor's identity plus that supervisor's own sequence number for it (for example `root.2.5`).<br>• Each supervisor numbers only its own children, while it processes its own requests, so no counter is shared.<br>• Numbers are never reused, so a restarted child gets a new identity.<br>• Order is lexicographic by address.<br>• Named actors compare by pid, not by name, as in Erlang.<br>**Decided:** a fixed-size encoding that fits 64 bits, so a pid goes on the BEAM wire as `NEW_PID_EXT`'s 32-bit id plus 32-bit serial, with no translation.<br>**Decided layout:** a 32-bit **supervisor position** and a 32-bit **child number**. This maps one-to-one onto `NEW_PID_EXT`'s id and serial.<br>• `bees_config` numbers supervisor positions at build time, in tree order, with the root supervisors first. A position is stable across its supervisor's restarts, and the position table maps it back to the full path.<br>• The owning supervisor assigns child numbers from 0 upward, and never reuses them. **Numbering continues across the supervisor's restarts**: the supervisor's parent keeps the high-water mark in its child-table row and hands it back on restart (decided).<br>• Limits: about 4.3 billion positions, and about 4.3 billion children per position over the node's life. Because numbering continues across restarts, an exhausted position stays exhausted.<br>• **Positions for supervisors started at run time** (decided). Every supervisor that can start supervisors at run time owns a **block of positions**. `bees_config` reserves the blocks at build time, because the compiler knows which supervisors start others. A supervisor started at run time gets a slice of its parent's block for any supervisors it starts in turn. Positions are handed out from the owner's own block, never reused, and no counter is shared.<br>• BEES's per-core spawn supervisors use the same blocks. Before one runs out of child numbers, BEES starts a replacement at a fresh position, sends new spawns there, and lets the old one drain as its processes finish.<br>• When a block runs out, its supervisor fails as if it had breached its restart intensity.<br>**Stale refs** (decided): a ref to a dead actor is only guaranteed never to equal a live actor, which never-reused numbers already give. BEES does not require stale refs to be detectable. Separately, it is Silica's concern that sending to a stale ref must not read a freed control block (the spec says such a send raises `actor_not_found`). | Compiled code compares pids constantly, and the D18 actor-to-supervisor map needs equality to find entries. Today an `actor_ref` is a pointer to a control block that is `free`d when the actor dies. A later spawn can reuse that address, so pointer identity can make a stale pid equal to a new actor. | A2, T1, B3 | Comparing `actor_ref_bits` words, which is unsafe because of address reuse | Spec §4.5.1, §7.4; ACB freed at teardown (`prims_actors_runtime_asm.silica`, around line 460); the crash report prints the pointer as `actor_id` |
| **S-27** | **Silica's bare-metal AArch64 path.** | Release 1.0 includes raw AArch64 as a platform. Silica has emitters only for Apple Silicon, Linux AArch64 and ESP32-S3; bare-metal AArch64 is a "later path" in Silica's ROADMAP. | Release 1.0 item 8 (raw AArch64) | ESP32-S3 is the only raw target | Silica ROADMAP, "Later paths"; `porting_for_os_free_targets.md` (design plan) |
| **S-28** | **Supervisor exit reports to a report sink.** A supervisor's flags may name a report sink (an `actor_ref`). After the supervisor handles a child's exit, whether or not it restarts the child, the runtime casts a fixed-shape report to that sink. The proposed shape is `(:child_exit, pid, failure_reason, restarted, new_pid)`. Reports are sent in supervision-ingress order. | With every actor supervised (D18) and no monitors (D20), supervisor reports are the only reliable news of a death (D23). BEES's exit hub needs them to remove names, delete owned tables, end BEES-level link partners, and tell BEAM-mode node connections about remote links. | A2, B3, I1 | None: without reports, links and remote links can only poll supervisors | Spec §15.4.9, §15.4.10 (exit notifications reach only the supervisor's own ingress) |

---

## 4. Native edge inventory

The native edge is one C archive, `libdangerous_bees_native.a`, reached through Silica's Fifi. It is built for each
target: against the OS on the hosted targets, and against the board on the raw targets, ESP32-S3 and AArch64 (Fifi
there is S-24). Silica code reaches it only through `dangerous_bees_*` wrapper modules and `spawn_dangerous` workers,
and every value it returns is re-created (S-5) before anything else uses it. Network content then passes `bees_ingress`. Byte payloads cross the boundary as `buf(uint8)` (pointer and length) once S-21 lands, and as
records of `uint64` words until then; they are never Silica `string` values. Structured results cross as flat
records.

| Entry | Purpose | Retires when |
| --- | --- | --- |
| `clock_monotonic_ns`, `clock_system_ns`, `poll_wait(ms)` | Time and the timer tick | S-3 |
| TCP and UDP socket calls; kqueue/epoll registration and wait | `bees_io`, `prim_inet` | Silica's own TCP/IP implementation (S-22) |
| `getaddrinfo` (A/AAAA records only) | DNS | Silica's `resolve_hostname` (S-22) |
| File write, directory operations, file metadata | `prim_file` | Silica `device_io` completion |
| TLS 1.3 session operations through rustls (D6), with no sockets of its own: configure, handshake, read, write, peer certificate DER and its SHA-512, ALPN | TRUST, BEAM-mode TLS | A Silica TLS built-in (S-13; not planned at first, D17) |
| SHA-2 family, MD5, HMAC | `crypto`, certificate fingerprints, the Erlang cookie challenge | S-14 (MD5 stays for as long as BEAM mode exists) |
| `random_bytes(n)` | `crypto:strong_rand_bytes`, tokens, challenges | A Silica secure-random built-in (not planned at first, D17) |
| `ct_equal(a, b)` | Token validation | S-14 |

**Rules for the edge:**

- No handles are visible to Silica beyond connection identifiers owned by one worker actor.
- No callbacks.
- Every entry point enforces a hard limit on the size of its inputs.
- The archive is rebuilt from pinned sources, and its hash is recorded in the trial tree.

---

## 5. Not provided

BEES makes compiling a BEAM language to Silica possible, not easy (D19), so it does not duplicate every BEAM
construct. These are left out on purpose. Where a compiler wants something similar, it builds it from what Silica
and BEES do provide.

| Feature | Status | Reason |
| --- | --- | --- |
| Loading or interpreting `.beam` bytecode at run time | Not provided | Code is compiled ahead of time by the language compilers. |
| Catching failures (`catch throw:…`, `catch error:…`, `catch exit:…`, `after` clauses, Erlang's `catch Expr`) | Not provided (D2); compilers reject them at compile time | Every failure, `throw` included, fails the actor, and its supervisor handles it. Silica aims to catch failures at compile time. BEES offers result-returning versions of commonly caught BIFs. |
| `trap_exit` for ordinary processes | Not provided (D18) | In Silica only supervisors trap exits, and `exit/2` goes to the target's supervisor. |
| Monitors (`erlang:monitor/2`, `spawn_monitor`, `monitor_node`, Elixir's `Process.monitor`) | Not provided (D20). In BEAM mode, monitor requests from OTP peers are dropped (D21). | A pending Silica `call` already wakes with the callee's death result, and supervisors handle fate-sharing. Other uses, such as cleaning up after a client that dies, are for compilers to build from what Silica and BEES provide. |
| BEAM-shaped failure messages (`{'DOWN', …}`, `{'EXIT', …}`, term exit reasons) | Not provided (D18) | Compiled code sees Silica's own shapes and atom reasons. At a BEAM-mode node boundary, BEES translates reasons to and from Erlang terms on the wire. |
| Processes without a supervisor | Not provided (D18) | BEES requires every actor to have a supervisor. |
| Creating atoms at run time | Not provided. `list_to_atom` and `binary_to_atom` return only atoms the atom lookup holds (D16); the wire accepts only those atoms too. | Decided in D16, for security. Atoms are Silica atoms. Silica's atom table is fixed at compile time, and BEES has no atom table of its own. In BEAM mode, the same rule makes the set of peers that may connect fixed at build time: a node must be named in the code. |
| Calling a local fun received from an Erlang node in BEAM mode | Not provided. The fun is carried as an opaque term, and calling it raises `badfun`. | Its code is BEAM bytecode. Export funs (`fun M:F/A`) work through the module table. |
| Hot code loading | Post-1.0 (S-12) | Needs Silica dynamic linking. |
| NIFs loaded at run time (`erlang:load_nif`) | Not provided (D13) | Each compiler reworks a NIF into a Silica actor that wraps the external call. |
| ETS concurrency guarantees | The options are accepted, and the actual semantics are documented | Silica actors share no mutable memory, so every table access is serialized through the table's owner. |
| Emulator introspection (`erts_debug`, scheduler `system_flag`s, GC statistics) | Accepted as no-ops or mapped (`garbage_collect` triggers an evacuation), and documented | These describe emulator internals that do not exist here. |
| Bug-for-bug OTP compatibility | Not claimed | As the README states; semantics follow one named OTP release (D14). |
