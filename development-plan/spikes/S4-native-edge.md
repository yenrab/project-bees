# Spike S4: Native edge

**Status:** run 2026-09-17 on `silica-999977-macos-applesilicon`. **The edge works**, in a narrower
shape than the tutorial describes. Four Silica findings (SD-15 to SD-18).
**Trial:** `trials/s04_native_edge/dangerous_clock_and_poll.silica`; archive from `native/`.

## 1. What was built

- `native/`: `libsilica_bees_native.a` from `src/silica_bees_native.c`, with the sidecar
  `dangerous_exposure_source/bees/silica_bees_native_wrapper.meta`. Entries:
  `bees_clock_monotonic_ns`, `bees_clock_system_ns`, `bees_poll_wait_ms(ms)`. Built by `make -C native`.
- A trial whose `spawn_dangerous` worker reads the monotonic clock, waits 50 ms in `poll`, reads
  the clock and the wall clock, and casts the three raw values to a sink that judges each inside
  the handler that receives it (FFI spec §7.6). Output `true true true / done`.
- `tools/run_trial.py`: the harness runner for programs ending in `wait_for_exit()`, which feeds
  `exit` after a marker line (a `NAME.wait_for_exit` file names the marker), as Silica's own FFI
  trials do.

## 2. The shape Silica accepts (and what the naming cascade looks like)

| Tutorial shape | What the compiler does |
| --- | --- |
| A `dangerous_*` library unit with foreign declarations and exported adapter functions (`fn add(l, r) { add_raw(l, r) }`) | Compiles, and **faults at run time** when the adapter is called (SD-15). Silica's own trials never call C from an adapter: their exported `add` computes `left + right`. |
| The library unit exports the worker behaviour; the app calls `spawn_dangerous(state, lib@worker)` | **E4041** in the library unit: an `external_danger` sequence is valid only in a behaviour whose `spawn_dangerous` site the compiler can see, which it looks for in the same unit (SD-17). |
| The library unit exports an installer that calls `spawn_dangerous` itself | **E4042** at the app's call: no `dangerous_*` function may be called outside a worker's `external_danger` sequence (SD-17). |
| Foreign declarations, worker and `main` in one unit | **Works.** This is what every Silica FFI trial does. |

So in FP1 the edge's Silica face cannot be a library unit: each program's `dangerous_*` root module
must contain the foreign declarations and the worker behaviours, and spawn the workers itself.
`src/shim/dangerous_bees_native.silica` records the intended library shape and is marked as not
compilable as such. `bees_config` can generate the root module's edge section from it when the
time comes; whether that is a design change or a build detail is for the user (see §4).

The cascade itself is as described: `W4001` once per accepted binding, and every unit that
imports a `dangerous_*` unit must be `dangerous_*`.

## 3. Runtime findings

- **A worker answers one request only (SD-18):** the second message to a `spawn_dangerous` worker
  faults in `os_unfair_lock_lock`. Any number of foreign calls within one dispatch work. The
  workaround is a fresh worker per request, which TRUST's per-request worker and D13's NIF worker
  already are; a clock or timer source will batch what it needs into one request.
- `print_bool` inside a `case`-arm block loses its helper and the assembler fails (SD-16); print
  from a top-level sequence.
- `poll` and `clock_gettime` behave: a 50 ms wait measures 50–150 ms on both clocks.

## 4. Consequences

- The retirement triggers in gap ledger §4 stand; the archive and sidecar are the shape B1 and
  A3 build on.
- **Open:** how the edge's Silica text reaches each program while SD-17 stands. The choices are a
  `bees_config`-generated root module (a build step; fits "every app has a root module named
  `dangerous_*`", roadmap §4.7) or waiting for Silica to accept cross-unit workers.
