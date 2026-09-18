# Silica defects found while building BEES

Findings about the Silica toolchain that are not Track S items: defects and surprises in the
fixed-point compiler that BEES works around. Each is filed with Silica by a person (AI_POLICY.md);
the trial named here is the reproducer. Compiler generation is in `tools/silica.generation`.

| # | Found | What | Reproducer | BEES workaround |
| --- | --- | --- | --- | --- |
| SD-1 | 2026-09-17 | A cross-module call with **too many** arguments compiles. `answer/1` called as `mod@answer(1, 2)` passes; too few arguments is rejected (E2005), as in Silica's own `qualified_call_wrong_arg_count_trial`. Same on `silica-999977`, `silica-999982` and `silica-gen2`. | `trials/r01_harness/fail_wrong_arity.silica` (uses too few arguments so it fails as intended; the comment records the too-many case) | None needed; noted so that a BEES trial never relies on the compiler catching extra arguments. |
| SD-2 | 2026-09-17 | `project_makefiles/Makefile` links without `-Wl,-e,main`, so a program built with the drop-in Makefiles fails to link ("Undefined symbols: _main"). The trial makefiles pass the flag. | Any program built with `project_makefiles/` | `tools/silica.mk` passes `-Wl,-e,main`. |
