# Spike S3: Actor ceiling

**Status:** run 2026-09-17 on `silica-999977-macos-applesilicon`, Apple Silicon, 16 GB, 10 cores.
**Trials:** `trials/s03_actor_ceiling/`.

| Measurement | Result |
| --- | --- |
| Maximum live actors in one process | **4,093.** Every actor is a pthread with a 1 GiB stack reservation (`ssize=0x40000000` in fault reports), and macOS caps a task at `kern.num_taskthreads` = 4,096. Beyond that `spawn` returns a ref whose `cast` reports false; nothing crashes. |
| Resident memory per idle actor | ~17 KB (4,000 actors: 69 MB RSS). |
| Virtual address space per actor | 1 GiB reserved (stack); not a limit at 4,096 actors. |
| Cost of `spawn` plus one `cast` | ~70 µs (4,000 in 0.30 s), dominated by thread creation. |
| Cost of one `cast` to a live actor | < 1 µs (100,000 in ~0.07 s, spike S6's measurement). |
| Cost of one `call` round trip | ~5 µs (200,000 in 0.98 s, 0.93 s of it system time). |

**Consequences.** The PoC scale is "thousands, not millions" as roadmap §4.3 assumes, and the exact
ceiling is ~4,000 processes per node on macOS. S-6 (many actors per carrier thread) is what
removes it; nothing BEES can do raises a kernel thread cap. `spawn` at 70 µs also sets the floor for
D18's supervised spawn, which spike S13 measures on top of this.
