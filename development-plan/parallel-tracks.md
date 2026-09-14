# BEES Development — Parallel Tracks

BEES exists so that programs written in **BEAM languages** (Erlang, Elixir, Gleam, LFE) can be **compiled to Silica
and its constructs**. It is a shim: it supplies what compiled BEAM code needs and Silica lacks. It is not a copy of
the BEAM's code API for Silica programmers. Each BEAM language has its own language-to-Silica compiler. Those
compilers are **separate projects, outside BEES**. BEES processes are plain Silica actors.

Two other work streams sit beside the runtime tracks described here, and both are described in the
[roadmap](roadmap.md):

- **Track T**, the *target contract* that every language compiler targets.
- **Track S**, prerequisites delivered in Silica itself.

The runtime development of Project BEES is organized into **two distinct, independently-progressing tracks**:

1. The **on-node** track — everything that lives and runs inside a single machine.
2. The **inter-nodal** track — everything that lives and runs **between** machines.

> **These tracks are not sequential.** Neither one is a prerequisite for the other to begin. They can — and should — be developed **in parallel**, with periodic integration points where their interfaces meet.

The two tracks share a common vocabulary (actors, messages, links, supervision) and a common substrate (Silica), but they own **separate problem domains**, **separate code paths**, and **separate milestones**. A contributor can make meaningful progress on one track without blocking on the other.

---

## Track A — On-Node

**Scope:** the runtime shim that compiled BEAM-language code needs on a single host.

This track is concerned with how BEAM processes, which are Silica actors, are *created*, *scheduled*, *balanced*, *preempted*, *linked*, *supervised*, and *torn down* on one machine, and with the BEAM-specific runtime semantics that compiled code relies on. Its success criterion is that a program written in a BEAM language, compiled to Silica, runs correctly and efficiently on a single node, using all available cores, with the scheduling and lifecycle behavior BEAM programmers expect.

### Concerns owned by this track

- **Terms & memory** — the universal term type compiled code manipulates, Erlang term order, maps, binaries and bit syntax, and reclaiming process memory without a garbage collector. Atoms are not a BEES concern: a BEAM atom is a Silica atom, held in Silica's atom table.
- **BIFs & ERTS-level modules** — `erlang`, `ets`, `persistent_term`, `os`, `code`, `crypto`, and the other modules the BEAM implements natively rather than in Erlang.
- **OTP behaviours on Silica constructs** — mapping `gen_server`, `gen_statem`, and `supervisor` modules onto Silica's gen_server, state-machine, and `Supervisor` traits.
- **Schedulers** — run queues, work-stealing, fairness, preemption points, reduction-style budgeting (or the Silica-appropriate analogue). The scheduler lives in BEES. On OS-hosted apps it runs plain Silica actors through a scheduler interface that the Silica runtime provides.
- **Multi-core balancing** — distribution of actors across cores, affinitization, locality, migration *between cores on the same node*. On OS-hosted apps the kernel owns the cores, so BEES balances BEAM-style between its carrier threads and a pin is at most a preference. Running raw on a chip, no OS shares the cores, so a pin is exclusive and BEES balances BEAM-style-ish directly onto the cores, using Silica's `migrate_actor()`, with each migration taking effect at a yield point.
- **Actor lifecycle** — spawn, exit, normal/abnormal termination, links, monitors, trapping, mailbox semantics.
- **Supervision integration** — how BEES surfaces interact with Silica's native supervisors and actors.
- **Mailboxes & message delivery (intra-node)** — ordering guarantees, selective receive semantics, back-pressure for in-process sends.
- **I/O integration on the host** — how actors interact with the host's I/O without breaking scheduling assumptions.
- **Local naming / registry** — process registries, named actors, local addressing.
- **Observability of a single node** — tracing, introspection, metrics for what is happening on one machine.

### What a track-A contributor does *not* need to solve

- Network transports, node discovery, cross-node identity, or cluster membership.
- Wire formats for messages leaving the machine.
- Failure semantics that span more than one host.

The on-node surface is expected to be **self-consistent and useful on its own**, even before any inter-nodal work has shipped.

---

## Track B — Inter-Nodal

**Scope:** the distribution-oriented surface between BEES nodes.

This track is concerned with how nodes find each other, how actors are *named* and *addressed* across the network, how messages travel between nodes, and how placement and migration work when "elsewhere" is not just another core but another machine. Its success criterion is that two or more BEES nodes can cooperate as a cluster, exchanging messages and coordinating placement, on top of whatever on-node surface exists at the time, and that BEES nodes can join existing Erlang/OTP clusters when an operator explicitly enables standard BEAM distribution.

### Concerns owned by this track

- **Node identity & discovery** — how a node names itself, how nodes find each other, how membership changes are observed.
- **Cross-node addressing** — referring to an actor that lives on another node; stability of those references across migration.
- **Transport** — protocols and adapters for moving messages between nodes; framing, ordering, and back-pressure across the wire.
- **Placement & migration (inter-node)** — policies and mechanisms for deciding *which* node an actor should run on, and for moving it.
- **Partition & failure semantics** — what happens to references, links, and monitors when a peer becomes unreachable.
- **Security & trust at the boundary** — authentication, authorization, and confidentiality between nodes, **secure by default** (see *Security posture* below).
- **Observability across nodes** — cluster-level tracing, distributed metrics, debugging conversations that cross hosts.

### What a track-B contributor does *not* need to solve

- Per-core scheduling, preemption, or local mailbox internals.
- Single-node supervision trees or local registries.
- The shape of the actor API as it appears to user code on one machine.

The inter-nodal surface is expected to be **developed against a stable contract** with the on-node surface, not against its internals. As long as a node can spawn actors, deliver local messages, and report lifecycle events, the inter-nodal track can make progress against it.

### Security posture — secure by default

The BEAM was designed in an era when **inter-node interaction was not under continuous attack**. Distribution between Erlang nodes was conceived for trusted LANs and operator-controlled deployments, and the defaults reflect that assumption: a shared cookie, cleartext distribution unless explicitly upgraded, and broad cross-node capabilities once two nodes consider themselves connected. That assumption no longer matches the environment most modern systems run in.

**BEES will be designed to be more secure than the BEAM by default.** Concretely, the inter-nodal track treats the network as hostile from day one. The defaults are intended to include:

- **Mutual authentication between nodes** as a precondition for distribution, not an opt-in.
- **Encrypted transport** for inter-node traffic by default.
- **Least-authority cross-node operations** — the set of things a remote peer can ask of a node is narrower than "anything a local actor could do," and is opened up explicitly rather than implicitly.
- **Identity tied to verifiable credentials**, not to a shared secret that could be construed as the entire trust model.
- **Auditability** — inter-nodal actions a security reviewer would want to see should be observable through the same observability surface used for everything else.

These specifics will evolve with the implementation; what is fixed is the **stance**: in BEES, the secure path is the default path, and the insecure path is the one you have to ask for.

#### Opting down to BEAM-equivalent security

There will be a **supported way to reduce the security posture down to what the BEAM currently does** — for example, running on a trusted LAN, reproducing legacy behavior, or interoperating with existing Erlang/OTP clusters. This mode is:

- **Off by default.**
- **Explicit** — it must be configured, and it should be obvious in configuration and in logs that a node is running in this mode.
- **Scoped** — granular where practical, so an operator can relax one property (e.g. transport encryption) without relaxing all of them.
- **Documented as a downgrade**, not as a peer to the default. The default is the recommendation.

The goal is parity *as a ceiling*, not as a baseline: BEES users should be able to match BEAM's posture when they need to, but the path of least resistance should be the more secure one.

---

## Why two tracks, in parallel

- **Different expertise.** Schedulers, preemption, and mailbox semantics are a different discipline from network protocols, partition handling, and cluster membership. Forcing them into one queue serializes work that does not need to be serialized.
- **Different failure modes.** On-node bugs are typically deterministic and reproducible on a developer's laptop. Inter-nodal bugs are concurrency- and network-shaped. Their tooling, tests, and review checklists differ.
- **Different release cadence.** The on-node surface can ship useful behavior to single-node users long before a full distribution story exists, and the inter-nodal surface can mature against successive on-node versions without blocking either side.
- **Honest scope.** Project BEES has, from its inception, called out **multi-core** *and* **networking** as first-class. Treating them as parallel tracks reflects that intent in how the work is actually organized.

---

## Where the tracks meet

Although the two tracks proceed in parallel, they are not unrelated. They meet at a small, deliberate set of contracts:

- **Actor identity** — a representation of "who" an actor is that is meaningful both locally (Track A) and across the network (Track B).
- **Message envelope** — a shape for a delivered message that does not change depending on whether the sender was local or remote.
- **Lifecycle events** — spawn, exit, link-broken, monitor-fired — surfaced uniformly so that distribution can react to them without reaching into scheduler internals.
- **Placement hooks** — the point at which "run this actor *somewhere*" is resolved; Track A answers "which core," Track B answers "which node," and the hook itself is shared.

These contracts are the **integration surface** between the tracks. Changes to them are coordinated; changes *behind* them are each track's own business.

---

## What both tracks depend on: compilation and Silica prerequisites

Both runtime tracks serve code produced by the language-to-Silica compilers, which are outside BEES. **Track T**'s
*target contract* specifies, for every BEAM construct, which Silica construct or shim component the generated code
targets and what each compiler must emit. The contract is the fifth integration surface, alongside the four above. It
is also how code compiled from different languages interoperates within one program.

Some things that compiled BEAM code needs belong in **Silica itself** rather than in a library:

- constructs Silica's spec promises but its runtime does not yet deliver (links, monitors, lightweight actors on carrier threads, byte buffers across the FFI boundary);
- constructs that belong beside what Silica already has (gen_server and state machines, beside the `Supervisor` trait; Silica's own TCP/IP implementation);
- language primitives (big integers, region release).

Byte handling is not one of them. Silica has no type conversion and BEES adds none, so each language compiler lowers bit syntax with lookups it generates itself.

The [roadmap](roadmap.md) tracks these as **Track S**, carried out in the Silica repository under Silica's own rules. Every milestone lists the Track S items it blocks on. The full construct mapping is in the [gap ledger](gap-ledger.md). The two distribution modes are designed in [inter-nodal-modes.md](inter-nodal-modes.md).

---

## Summary


|                                  | Track A — On-Node                                                   | Track B — Inter-Nodal                                                  |
| -------------------------------- | ------------------------------------------------------------------- | ---------------------------------------------------------------------- |
| **Domain**                       | One machine                                                         | Between machines                                                       |
| **Primary concerns**             | Terms and memory, processes and lifecycle, BIFs, OTP behaviours on Silica traits, scheduling policy | Node identity, transport, cross-node addressing, placement, partitions |
| **Depends on the other?**        | No                                                                  | No                                                                     |
| **Can ship useful value alone?** | Yes (compiled BEAM-language programs on one node)                   | Yes (against any conforming on-node surface)                           |
| **Meets the other at**           | Actor identity, message envelope, lifecycle events, placement hooks | Same                                                                   |


Both tracks are active from day one. Progress on either is progress on BEES.