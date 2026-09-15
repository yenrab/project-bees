# Spike S12: The TLS Engine (rustls Everywhere)

**Status:** planned, not run. This document specifies the spike. Nothing has been built yet.
**Decision it confirms:** D6, rustls everywhere ([roadmap §6](../roadmap.md#6-decisions)).
**Fallback if the raw targets fail:** Mbed TLS on ESP32-S3 and bare-metal AArch64 only, behind the same C interface.

---

## 1. Purpose

TRUST can't run without TLS, so the TLS engine is on the critical path for B1. D6 chose rustls on every target. This
spike checks that choice before B1 depends on it. It answers four questions:

1. Can rustls, behind a small C interface, meet the TRUST TLS profile on all four 1.0 targets?
2. Does it fit BEES's I/O model: no sockets of its own, no callbacks into Silica, and every output re-created?
3. What does it cost in code size and RAM, especially on the ESP32-S3?
4. How mature is the crypto provider written in pure Rust that the raw targets need?

## 2. Requirements

These come from the TRUST profile (inter-nodal-modes §3.2), D11, D17 and the native-edge rules (gap ledger §4).

| # | Requirement | How the spike checks it |
| --- | --- | --- |
| R1 | TLS 1.3 only; TLS 1.2 refused | A client that offers only TLS 1.2 fails the handshake |
| R2 | Both sides present certificates (mTLS); the server requires a client certificate | A client with no certificate is refused; a client certificate from another CA is refused |
| R3 | Key exchange **X25519MLKEM768** only; classical-only peers refused | The negotiated group is reported as `0x11EC`; a client offering only X25519 is refused |
| R4 | Ciphers AES-256-GCM or ChaCha20-Poly1305 only | The negotiated suite is reported; the configuration allows no others |
| R5 | Certificates Ed25519 or ECDSA P-256 | Both kinds of certificate complete the handshake |
| R6 | No 0-RTT, renegotiation or compression | Configuration review (rustls doesn't implement renegotiation or compression) |
| R7 | ALPN `trust/1`, enforced | A peer offering another ALPN is refused; a peer offering none is refused by the interface's check after the handshake |
| R8 | The peer's certificate DER and its SHA-512 are available (D11), and so are the client-side pins | The interface returns both, and the fingerprint matches one computed separately |
| R9 | No sockets of its own; ciphertext in, ciphertext and plaintext out | The test harness pumps bytes between two endpoints through memory buffers only |
| R10 | No callbacks into Silica (Fifi only calls outward) | Interface review: every function is called by Silica and returns |
| R11 | Every output can be re-created (S-5) | Outputs are flat byte buffers with explicit lengths, plus scalars |
| R12 | Builds for all four 1.0 targets | Per-target build (§5) |
| R13 | Fits the ESP32-S3 (about 512 KB SRAM) | Code size, and peak RAM during a handshake and a 1 MiB transfer |
| R14 | A secure random source and a clock on the raw targets | These come from the board's hardware RNG and clock through the native edge (D17) |
| R15 | Apache-2.0-compatible license | rustls is Apache-2.0, ISC or MIT; each provider crate is checked the same way |

## 3. The C interface (`dangerous_bees_native` TLS entries)

The interface owns no sockets and makes no callbacks. Silica reads and writes the network through its own TCP/IP
(S-22). It hands incoming ciphertext to the engine, takes outgoing ciphertext back, and re-creates everything the
engine returns (S-5) before the data is used or written to a socket.

```c
/* Configuration: PEM inputs. The profile (TLS 1.3, X25519MLKEM768, the two AEADs, ALPN "trust/1") is fixed inside. */
bees_tls_config *bees_tls_server_config_new(const uint8_t *cert_pem, size_t cert_len,
                                            const uint8_t *key_pem,  size_t key_len,
                                            const uint8_t *ca_pem,   size_t ca_len);   /* client CA; mTLS required */
bees_tls_config *bees_tls_client_config_new(const uint8_t *cert_pem, size_t cert_len,
                                            const uint8_t *key_pem,  size_t key_len,
                                            const uint8_t *ca_pem,   size_t ca_len);   /* server CA */
void             bees_tls_config_free(bees_tls_config *);

/* Connections. */
bees_tls_conn *bees_tls_server_new(const bees_tls_config *);
bees_tls_conn *bees_tls_client_new(const bees_tls_config *, const uint8_t *server_name, size_t len);
void           bees_tls_conn_free(bees_tls_conn *);

/* Ciphertext in and out: no sockets. */
int bees_tls_feed(bees_tls_conn *, const uint8_t *in, size_t len, size_t *consumed);  /* ciphertext from the peer */
int bees_tls_pull(bees_tls_conn *, uint8_t *out, size_t cap, size_t *written);        /* ciphertext for the peer */

/* Plaintext. */
int bees_tls_write(bees_tls_conn *, const uint8_t *in, size_t len, size_t *accepted);
int bees_tls_read (bees_tls_conn *, uint8_t *out, size_t cap, size_t *written);

/* State and peer information, available once the handshake completes. */
int bees_tls_handshake_done(const bees_tls_conn *);
int bees_tls_peer_cert_der(const bees_tls_conn *, uint8_t *out, size_t cap, size_t *written);
int bees_tls_peer_fingerprint_sha512(const bees_tls_conn *, uint8_t out[64]);
int bees_tls_alpn(const bees_tls_conn *, uint8_t *out, size_t cap, size_t *written);  /* must equal "trust/1" */
int bees_tls_kx_group(const bees_tls_conn *);        /* IANA group id; 0x11EC = X25519MLKEM768 */
int bees_tls_cipher_suite(const bees_tls_conn *);    /* IANA suite id */
```

Rules for the interface:
- Every function enforces hard size limits on its inputs.
- Status codes are small integers, never strings, and carry no peer-supplied detail.
- A handle (`bees_tls_conn *`) stays inside the one `spawn_dangerous` worker that owns the connection. It is never
  sent to another actor, following the native-edge rules in gap ledger §4.
- Once the byte-buffers-across-FFI item (S-21) lands, `buf(uint8)` crosses the boundary. Until then, bytes are
  packed in records of `uint64` words.

These spike-only hooks (a classical-only client, a client with no certificate, a client that offers TLS 1.2 only)
are built only in the test harness. They are never part of the edge's release build.

## 4. Crypto providers per target

| Target | rustls build | Crypto provider | Notes |
| --- | --- | --- | --- |
| macOS AArch64 | With `std` | `aws-lc-rs` | rustls 0.23's default features include `aws_lc_rs` and `prefer-post-quantum`. The spike turns off `tls12`, so only TLS 1.3 is built in. It also narrows the key-exchange groups to X25519MLKEM768 and the cipher suites to R4. Building `aws-lc-rs` needs `cmake` and a C compiler. |
| Linux AArch64 | With `std` | `aws-lc-rs` | As for macOS. Needs a Linux AArch64 machine, or a cross sysroot for `aws-lc-sys`. |
| Bare-metal AArch64 | Without `std` (`alloc` only) | Pure Rust (see below) | Target `aarch64-unknown-none`. It is run under QEMU's `virt` board. Needs a clock and a random source from the board (D17) and an allocator. |
| ESP32-S3 | Without `std` (`alloc` only) | Pure Rust (see below) | Target `xtensa-esp32s3-none-elf`, which needs the esp-rs Rust toolchain for Xtensa. The clock and random source come from the board's hardware (D17). |

**The pure-Rust provider is the main risk.** `aws-lc-rs` needs an operating system, so the raw targets need a
provider written in pure Rust. There are two paths:
- **`rustls-rustcrypto`**, the existing provider. It is at version `0.0.2-alpha` on crates.io, an early alpha. The
  spike checks whether it supports X25519MLKEM768 and whether its implementations are fit for release.
- **A provider assembled from RustCrypto crates.** These include `ml-kem` (0.3.2, pure Rust), X25519, AES-GCM,
  ChaCha20-Poly1305, SHA-2, and ECDSA P-256 / Ed25519 verification. Combining ML-KEM and X25519 into the hybrid
  group is cryptographic glue code. It is small, but it needs an independent review before release, in keeping
  with "don't roll your own crypto".

Whichever path passes, the release criteria require the raw-target provider to be inside the external security
review (roadmap Release 1.0, item 6).

## 5. How to run it (for whoever runs the spike)

| Target | Where | Installs needed (not done yet) |
| --- | --- | --- |
| macOS AArch64 | This Mac | The Rust crates (`rustls`, `aws-lc-rs`, `rcgen` for test certificates). `cmake` and `clang` are already present. |
| Linux AArch64 | A Linux AArch64 host, such as the one used for Silica's Linux port | The Rust crates |
| Bare-metal AArch64 | This Mac, under `qemu-system-aarch64` (already installed) | `rustup target add aarch64-unknown-none`, a pure-Rust provider, a minimal board support package (allocator, clock, random source) |
| ESP32-S3 | This Mac plus an ESP32-S3 board | The esp-rs toolchain (`espup`), `espflash`, a pure-Rust provider. The ESP-IDF tools already under `~/.espressif` are not enough on their own. |

The functional test pumps bytes between a client and a server entirely in memory (R9). It runs the positive case
and each negative case in §2. It then records the negotiated group and suite, the ALPN, and the peer fingerprint.

## 6. Measurements to record

- Handshake time and bytes exchanged. The hybrid group adds about 2 KB to the handshake, compared with X25519 alone.
- Throughput for a 1 MiB transfer in each direction.
- Size of the static library and of a minimal linked program, per target.
- Peak RAM during the handshake and the transfer, on the ESP32-S3 and on the bare-metal AArch64 board.

## 7. Exit criteria and decision rule

- **rustls everywhere is confirmed** if R1–R15 pass on all four targets and the ESP32-S3 has room left for BEES.
- **If only the raw targets fail**, for example because the provider is too immature, the build is too large or
  X25519MLKEM768 is missing, the fallback applies. Mbed TLS takes over on the raw targets behind the same C
  interface. Its support for X25519MLKEM768 is then the first thing to check, because the TRUST profile needs it.
- **If the hosted targets fail**, D6 is reopened.

## 8. What is known before running

Taken from crates.io metadata on 2026-09-15. Nothing has been built.

- The latest stable rustls is **0.23.45**, licensed Apache-2.0, ISC or MIT. Its default features include `aws_lc_rs`
  and `prefer-post-quantum`, and it has a `ring` alternative and a `custom-provider` feature. A `0.24.0-dev`
  pre-release also exists.
- **`rustls-rustcrypto` is at `0.0.2-alpha`,** which confirms the provider risk in §4.
- **`ml-kem` 0.3.2** is available as a pure-Rust ML-KEM implementation.
