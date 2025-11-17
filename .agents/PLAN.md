## Input Parser Batching Plan (2025-11-17)
1. [x] Reconfirm goal: cut redundant `screen_write_collect_end` calls during escape-sequence parsing without altering terminal semantics.
2. [x] Capture current AGENTS instructions/constraints before editing `input.c`.
3. [x] Review current flush logic in `input_parse` and note when it fires.
4. [x] Inspect `screen_write_collect_*` helpers to understand batching expectations.
5. [x] Catalog transition handlers involved in escape parsing to classify which truly need a flush.
6. [x] Choose the heuristic: only flush while in ground state when handling non-print transitions; verify against tables.
7. [x] Implement the new condition (with rationale comment) in `input_parse`.
8. [x] Audit other `screen_write_collect_end` callsites for necessary changes (none found).
9. [x] Build `input.o` to validate the change compiles cleanly.
10. [x] Reason through ESC/C0/UTF-8 paths to confirm the new batching rule is safe.
11. [ ] Record findings in PLAN.md (this file) once work is complete.
12. [ ] Ensure inline comments/code clearly explain the optimization rationale.
13. [ ] Consider lightweight testing or describe manual validation strategy.
14. [ ] Summarize changes and expected impact for the user.
15. [ ] List any follow-up ideas (e.g., instrumentation of collect flush counts).

### Findings Log
- Adjusted `input_parse` so `screen_write_collect_end` only fires while the parser is still in `input_state_ground` and encounters a non-print transition, preventing CSI/OSC parameter bytes from flushing the batch buffer.

### Measurement / Validation Ideas
- Compare `ktrace`/`strace` output before/after to count `tty_write` batches during `cat` of large logs.
- Add temporary logging of `screen_write_collect_end` counts for profiling builds to quantify reduction.
- Manual validation idea: capture `tmux -vv` logs while replaying a large escape-heavy trace to ensure collect counts drop without affecting rendering.

### Follow-up Ideas
- Gate `screen_write_collect_end` with instrumentation counters to expose batching efficiency via `tmux display-message`.
- Investigate whether we can keep batching across certain control sequences (for example SGR that only changes colours) by deferring flush until the handler actually mutates screen state.
