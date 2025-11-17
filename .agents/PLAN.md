## Status Formatter + Job Dedup Plan (2025-11-17)
1. [x] Restate objective and constraints (AGENTS.md) for the status formatter/job optimization effort.
2. [x] Capture current git status to know baseline before edits.
3. [x] Re-read existing findings about status formatter/job hotspots to anchor the plan.
4. [x] Inspect `struct status_line`/`status_line_entry` definitions (status.c) to understand available caching hooks.
5. [x] Review `status_redraw` flow to identify where format trees are built and format jobs invoked.
6. [x] Study `format_job` structures/functions in format.c to map current lifetime and per-client linkage.
7. [x] Examine job notification path (`format_job_update/complete`) to see how clients get refreshed.
8. [x] Decide on a context-aware deduplication strategy (command string plus client/session/window/pane identity).
9. [x] Update `struct format_job` and the RB comparator to carry the new key fields.
10. [x] Teach `format_job_get` to populate the context key and reuse/refresh entries instead of spawning duplicates.
11. [x] Ensure job completion path notifies all interested clients and handles client disconnect cleanup.
12. [x] Audit impact on `format_free`/client teardown to avoid leaks; add removal hooks if needed.
13. [x] Rebuild relevant objects (`make format.o status.o`) to validate compile success.
14. [x] Consider runtime validation strategy (e.g., instrumentation counts, describing manual tests) since automated tests lack coverage.
15. [x] Update this PLAN.md with findings/results plus future ideas once implementation is done.
16. [x] Summarize changes for the user, highlighting performance impact and trade-offs.
17. [x] Propose follow-up tasks (additional caching, telemetry) to continue improving status formatter performance.

### Findings Log
- Format jobs are now keyed by the command template plus the resolved client/session/window/pane/paste-buffer context, allowing status redraws to reuse the same job instance instead of spawning one per format tree.

### Measurement / Validation Ideas
- Capture `tmux -vv` logs (or add temporary counters) to compare the number of `job_run` invocations before vs. after the change while a status line with multiple `#{run-shell}` entries refreshes.
- Profile `tmux` under a complex status configuration to ensure job counts stay flat as refresh frequency increases.

### Follow-up Ideas
- Share format jobs across different clients when their resolved command strings are identical by adding subscriber lists.
- Add cache metrics (e.g., `show-options -g @status-job-cache-hit`) so users can verify when jobs are being reused.
