# Spike S9: Reference lowerings

**Status:** run 2026-09-17 on `silica-999977-macos-applesilicon`. **All four programs run** as
hand-written Silica in the shape the draft contract says a compiler should emit. They are the first
worked examples for contract v0 (R0.3) and the seed of the conformance kit (T2).
**Trials:** `trials/s09_reference_lowerings/`; shim units `src/shim/bees_term.silica` and
`bees_recv.silica`, generated from `tools/gen_bees_term.py`.

## 1. The term (contract item 1, draft)

A term is a cell in a region: `(tag, n, f, a, p, h, t)`, spelled in every signature as
`(int64, int64, float64, atom, actor_ref, ref?(L, normal, rec), ref?(L, normal, rec))`.

| tag | term | slots |
| --- | --- | --- |
| 0 | `[]` | — |
| 1 | small integer | `n` |
| 2 | float | `f` |
| 3 | atom | `a` (from `bees_atoms`, D30) |
| 4 | local pid | `p` |
| 5 | cons | `h` head, `t` tail |
| 6 | tuple | `n` arity, `h` the element list |
| 7 | binary | `n` byte count, `h` a list of int cells 0..255 (a `buf(uint8)` slot needs a fixed size in the type; A1 decides) |
| 8 | fun | `n` module index, `h` = cons(fn index, environment list) |

The heap is the record `{ r: region(L, normal) }`, kept in a process's state as the field `h` and
passed **in** to every constructor; every constructor and accessor returns **one reference**,
`ref?(L, normal, rec)`. That shape is forced: an aggregate return leaks ~32 bytes of stack per call
for the life of the actor (SD-21), a plain `ref` return fails E2100, and a `(region, ref)` tuple
crashes (SD-20). A `ref?` cannot annotate a binding (SD-5), so results are nested as arguments, used
in `case`, or held in an inline record literal `{ c: … }`. `copy` evacuates a term into another
heap; `eq` is structural. Every region that holds terms is `region(L, normal)`, and a second region
in one scope rebinds `L` (SD-6).

A message is `{ r: region(L, normal), c: ref?(L, normal, rec) }`, a term in its own region, and
**every send site calls a builder** `msg_of(new_heap(), …)` that returns that record: a region bound
to a local name is freed at the scope's exit even after the cast moved it (SD-22).

Not yet possible: pid equality (`actor_ref_equal` is specified but links as undefined, S-26), so
`eq` on pids is `false`; maps and bignums (A1).

## 2. The four lowerings

| Program | Erlang | What it shows |
| --- | --- | --- |
| `ping_pong_selective_receive` | `ping` sends two pings, then `receive {pong, 2}` before `receive {pong, 1}` | **D1 reshaping.** `ping` is a cast-only actor with phases 0–3 in its state. Entering a receive first selects from the save queue (`bees_recv@select` moves the first message satisfying the clause predicate to the queue's head); a message matching no clause of the current receive is appended (`bees_recv@append`). `{pong, 1}` arrives first and is saved; the output is `pong 2 / pong 1 / done`. |
| `try_catch_rewrite` | `try list_to_existing_atom(S) catch error:badarg -> undefined end` | **D2.** The `try` becomes a call to the result-returning variant, here `bees_atoms@from_spelling`. |
| `funs_and_apply` | `twice(F, X) -> F(F(X))`, a fun literal, and `apply(m, twice, [F, X])` | **Contract item 4.** A fun is a tag-8 cell; the module's `apply_fun` is a `case` over its fn indices calling lifted closure bodies `(heap, env, args)`; its `apply` entry is a `case` over `(name, arity)`. |
| `binary_matching` | `parse(<<Len:8, Tag:16/big, Rest/binary>>)` | **§4.5.** Segments become reads of consecutive bytes and int64 arithmetic (`Tag = b1 * 256 + b2`), no conversion. |

## 3. Shapes a compiler must follow (found while writing these)

- A message is a heap record: a term in its own region, moved by the cast (§12.1.6). The receiver
  copies it into its heap (`bees_term@copy`) before keeping any part of it. The message region leaks
  today (SD-7).
- A process is a cast-only actor returning `(:no_reply, state)`; its state record holds its heap
  region, the save queue and the reshaped locals.
- `:none` as a call argument of type `ref?` is a type error; pass a term (`mk_nil`) or a typed field.
- Records built inside a `case`-arm block `{ … }` can lose fields (SD-16's class): work that builds
  records or sends goes in its own function, called from the arm.
- `head`, `tail`, `bytes` and the type names are reserved words: the shim uses `hd_of`, `tl_of`,
  `mk_*`; generated code must avoid them too (contract item 7).
- `case` arms whose bodies bind and print: keep `print_bool` out of case-arm blocks (SD-16).
- A `case` cannot be the expression under `produces pure`; bind it first.

## 4. What is still missing for Stage 1

- The processes are unsupervised (`spawn`), because SD-10 breaks record-state children under a
  supervisor; D18 waits on that fix.
- `after` timers (S-3 or the native clock through the edge), the process dictionary, and `exit/2`
  come with A2.
