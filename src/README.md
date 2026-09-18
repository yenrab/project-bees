# BEES sources

- `shim/` — the on-node shim (Track A): `bees_*.silica` units. Every module basename starts with
  `bees_` (roadmap §4.8).
- `inter_nodal/` — TRUST and BEAM-mode distribution (Track B).
- `contracts/` — the Silica side of the target contract: reference lowerings that compile and run.

Units are compiled only through trial leaves: a leaf's `lib/` holds symlinks to the units it
exercises, and `make integrate` at the root builds and runs everything (see `tools/trial.mk`).
