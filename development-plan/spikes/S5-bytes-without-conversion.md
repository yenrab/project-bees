# Spike S5: Bytes without conversion

**Status:** run 2026-09-17 on `silica-999977-macos-applesilicon`. **The technique works and its costs
are known.** Four Silica defects were found on the way (SD-11 to SD-14); all have design-neutral
workarounds. The int64-versus-float64 comparison question stays open.
**Trials:** `trials/s05_bytes_without_conversion/`.

## 1. Results

| Question | Answer |
| --- | --- |
| How are `uint8` literals written and matched? | As plain integers: `v: uint8 <- 203`, `case b of { 25 -> …; _: uint8 -> … }`. A `uint8` is written into `buf(L, normal, uint8, N)` with `write_buf`, read with `read_buf`. |
| Does a 256-way `case` compile to a jump table? | **No: a chain of 255 `CMP`/`B.NE`.** Measured at ~30 ns per lookup (1,000,000 in 0.03 s), which the branch predictor makes tolerable. |
| Byte ↔ `int64` | `u8_to_i64`: the 256-way case. `low_byte(n)`: `case n % 256` over 255 arms. An int64 round-trips through 8 little-endian bytes with `/ 256` and `* 256` only (`bytes_int64_bxor`). Negative numbers still need a two's-complement rule (not done here). |
| `bxor` on `uint64` | `(a bor b) band (bnot (a band b))`, correct (12 bxor 10 = 6). |
| `float64` decode from 8 bytes | Exponent and mantissa by int64 arithmetic on the bytes; the mantissa as a float by adding powers of two per set bit; scaling by `2^k` built from ten literals (`2^1 … 2^512`) by binary decomposition. All operations are exact. **~1.4 µs per decode.** |
| `float64` encode to 8 bytes | Exponent by a descending search over `2^e`, mantissa by peeling 52 bits off the scaled float, bytes by `low_byte`. Byte-exact against `struct.pack` for 1.0, −2.5, 0.1, 3.14159, 1e300, 0.0 and the smallest denormal. **~180 µs as written**; a binary search over the ten powers would bring it to about the decode cost. |
| `int64` ↔ `float64` comparison (`1 == 1.0`) | **Still to be determined.** The decode path shows the ingredients: an integer becomes an exact float through per-bit powers of two, and a float's integer part can be peeled off bit by bit; a comparison function built from them costs a few microseconds. Whether `bees_cmp` should pay that per comparison, or cache, is for A1. |

## 2. Shapes the emitter compiles correctly (SD-14)

The float work only compiles and runs with these rules, which BEES's codecs follow:

- A function that takes or returns `float64` is a **single expression**: no `sequence` block, since a
  float parameter does not survive a sequence binding or a call.
- A float parameter is **used at most once before any call**; a value needed several times is
  re-fetched (in BEES, re-read from its term cell).
- **Float literals in `case` arms** are calls to nullary functions (`fn one() -> float64 { 1.0 }`).
- Float literals in comparisons are bound first (SD-13); in arithmetic they are fine.
- Ints and floats meet only through calls: `u8_to_f64(b)` is `bits_f(u8_to_i64(b), 7)`.
- A byte written into a `uint8` buffer is bound first (SD-11).

## 3. Consequences

- Contract §4.5's technique stands: compilers can lower bit syntax and float encodings with lookups
  and same-type arithmetic. The 256-way `case` is the cost floor until Silica emits jump tables.
- ETF's `NEW_FLOAT_EXT` encoder and decoder (A1, B3) are the codec here, with a binary exponent
  search.
- **SD-12 matters more than the rest:** tail recursion consumes stack, so every compiled loop is
  bounded by the actor's stack within one dispatch. Until it is fixed, the contract must warn
  compilers, and BEES's own loops (ETF decode, list walks) stay short per dispatch.
