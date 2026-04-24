# Project BEES

**BEAM Elastic Execution Surface**

This file records the **intended scope** of Project BEES.

Project BEES is a **compatibility and execution layer** consisting of a set of files you can include in your Silica application. BEES presents a **BEAM-style surface** for BEAM-idiom programs: **processes as actors**, message passing, and the scheduling and supervision assumptions those models rely on. It is built **in Silica** and is meant to **sit where a conventional BEAM would**, while adding **first-class support for running and balancing work across many cores** and **across a network**—so placement, migration, and integration with I/O and clustering are not afterthoughts.

The name nods to **Bogdan and Björn’s Erlang Abstract Machine (BEAM)**. BEES is not a reimplementation of OTP in full, many parts of which exist in native Silica including actors and supervisors; it is a **focused execution surface** and **shim** that carries that lineage forward in a new substrate.

## Scope (in)

- **Execution surface** — the boundary at which “BEAM-like” code meets the host: schedulers, actor lifecycle hooks, links, and networking-aware execution (as the project defines them over time).
- **Multi-core** — **balancing and affinitizing actors** to cores, and policies for fairness, preemption, and locality, appropriate to a Silica-backed runtime.
- **Networking** — **distribution-oriented paths**: naming or addressing actors across nodes, and protocols or adapters for sending messages and coordinating placement (details evolve with the implementation).
- **Clear dependency on Silica** — BEES is expected to use Silica for compilation and any runtime pieces that are already in Silica; **its repository** is meant to track the **orchestration layer, APIs, and integration** that are specific to a BEAM-style surface rather than the whole compiler.
- **Documentation of intent** — what is guaranteed, what is compatible with Erlang/OTP concepts, and what is **Silica-specific** behavior.

## Scope (out) — for now

- Claiming **bug-for-bug, wire-format, or full OTP compatibility** with any particular Erlang/BEAM release without explicit, tested guarantees.
- Replacing the **entire** Silica project; BEES is a **sibling** product and repository whose role is the **actor execution and distribution** story on top of Silica.
- **Vendor-specific** deployment recipes unless they are clearly optional and documented as such.

(Adjust this section as the project ships concrete milestones.)

## Relationship to Silica

Silica provides the **language and toolchain** including actors and supervisors; Project BEES defines the **runtime-facing contract** and **infrastructure** for a BEAM-execution surface: scheduling, distribution hooks, and the glue between generated code and the machine. **Versioning** between Silica and BEES; The are no common version numbers between Silica and BEES. Each has its own release schedule.

## Status

Currently BEES is in its inception phase.

## License

Project BEES is licensed under the **Apache License, Version 2.0**.

## Contributing & heritage

When describing BEES publicly, it helps to **credit the BEAM and its origins** in one line, for example: *inspired by Bogdan and Björn’s Erlang Abstract Machine and the Erlang/OTP community.*
