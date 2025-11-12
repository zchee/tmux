## Performance Investigation Plan
- [x] Prepare plan persistence by recording the step list in .agents/PLAN.md.
- [x] Capture current repository status to ensure clean baseline for analysis.
- [x] List top-level repository contents to reaffirm module layout.
- [x] Search project docs or notes for existing performance discussions.
- [x] Inspect build configuration (configure.ac/Makefile) for optimization defaults.
- [x] Review tmux.h to refresh understanding of core data structures.
- [x] Examine server loop implementation to identify potential bottlenecks.
- [x] Inspect input handling pipeline for costly parsing or per-event overhead.
- [x] Analyze screen redraw and layout code for repeated expensive operations.
- [x] Assess buffer and grid management for inefficient algorithms or allocations.
- [x] Check window/pane update propagation logic for redundant work.
- [x] Evaluate job and hook subsystems for blocking operations in event loop.
- [x] Review logging and formatting utilities for hot-path overhead.
- [x] Identify opportunities for batching or caching frequently recomputed values.
- [x] Summarize observed performance risks with supporting evidence.
- [x] Recommend measurement and benchmarking strategies to validate improvements.
- [x] Update plan file with findings status and next actions.

## Next Actions
- Prioritize remediation of the input parser transition search and redraw batching.
- Prototype caching strategies for format evaluation before touching render loops.
- Validate proposed changes with targeted profiling to confirm impact.

## Hotspot Details
### Input Parser Transition Search
- **Reasons for poor performance:** `input_parse` linearly scans each state transition list per byte, thrashing caches and restarting lookups even when the same state repeats; it also forces frequent `screen_write_collect_end` calls that interrupt batching.
- **Reasons for potential performance improvements:** Precomputing constant-time dispatch tables (per-state arrays or binary search over sorted ranges) removes the O(n) scan, keeps transitions hot in L1, and lets us defer collection flushes until actual state changes.
- **Current plans:** Prototype a cached transition table keyed by byte value, validate with recorded terminal traces, and guard with regression tests capturing UTF-8 and escape edge cases.

#### Input Parser Transition Search Plan
- [x] Inspect `input_parse` and related transition structures to understand the existing lookup path.
- [x] Review current reuse of the last transition pointer and identify where repeated scans still occur.
- [x] Audit transition tables (including `INPUT_STATE_ANYWHERE`) for range coverage and sentinels.
- [x] Capture the design goal: constant-time transition resolution without altering parser semantics.
- [x] Compare caching approaches and confirm that per-state byte lookup tables fit within constraints.
- [x] Commit to building on-demand 256-entry caches keyed by `struct input_state *`.
- [x] Map integration points in `input_parse` to swap in the cached lookup while preserving batching checks.
- [x] Add helper scaffolding in code (via new function) to resolve transitions through the cache.
- [x] Ensure fallback behavior mirrors the existing fatal path when no transition is located.
- [x] Review ancillary functions (for example, `input_set_state`) for compatibility with cached results.
- [x] Update parser code accordingly and keep logging behavior unchanged.
- [x] Compile or run focused tests (where feasible) to validate correctness after changes.
- [x] Perform code-style and safety review on the new cache implementation.
- [x] Record outcomes and next steps in this plan and the task tracker.
- [x] Sync completion state with the planning tool once the work is finished.

### Pane Redraw Pipeline
- **Reasons for poor performance:** `screen_redraw_draw_pane` re-evaluates clip bounds and logs each line while `server_client_check_redraw` repeatedly walks panes, so redraws scale with pane count × line count even when content is static.
- **Reasons for potential performance improvements:** Caching visible rectangles, batching adjacent rows into single `tty_draw_line` calls, and suppressing debug logs during normal operation shrink redraw work and improve locality.
- **Current plans:** Introduce a per-pane redraw descriptor computed once per frame, adjust `tty_draw_line` usage to accept spans, and benchmark with tiled layouts under heavy output.

#### Pane Redraw Batching Plan
- [x] Review `screen_redraw_draw_pane` and related helpers to understand current bounds checks.
- [x] Define a per-pane geometry descriptor capturing visible source/destination ranges.
- [x] Refactor redraw logic to compute geometry once per pane and reuse it for all rows.
- [x] Adjust row iteration to skip non-visible rows without per-line conditionals.
- [x] Ensure colour defaults and palette handling remain correct with the cached descriptor.
- [x] Confirm overlays, scrollbars, and SIXEL drawing continue to function.
- [x] Reduce redundant debug logging or guard it appropriately.
- [x] Build and run relevant tests (e.g., `make tmux`) to validate compilation.
- [x] Profile or reason about expected reduction in per-line overhead.
- [x] Record outcome and follow-up tasks in this plan.

### Status Formatter and Jobs
- **Reasons for poor performance:** Every status update rebuilds format trees, repeats option lookups, and may spawn external format jobs, leading to excessive allocations and timer wakeups.
- **Reasons for potential performance improvements:** Memoizing option values, reusing format trees across frames, and sharing job executions limit churn and let status refresh frequency dominate actual UI changes.
- **Current plans:** Add a cache layer keyed by format string and context, coalesce format jobs behind a shared scheduler, and measure gains with complex status configurations.

### Command Queue Drain Loop
- **Reasons for poor performance:** `server_loop` polls `cmdq_next` for every client even when queues are empty, calling `time(NULL)` and emitting logs each iteration, which consumes CPU at idle.
- **Reasons for potential performance improvements:** Converting queues to event-driven wakeups and rate-limiting logging avoids busy spinning and reduces syscalls.
- **Current plans:** Wire queue insertions to libevent notifications, gate debug logging behind higher verbosity, and confirm idle CPU drops in perf profiles.

### Multi-Pass Client Window Maintenance
- **Reasons for poor performance:** `server_client_loop` iterates windows three times per tick (resize, reset flags, theme updates), touching every pane regardless of changes.
- **Reasons for potential performance improvements:** Consolidating passes and tracking dirty windows/panes lets us skip untouched panes and defer theme broadcasts until data changes.
- **Current plans:** Maintain per-window dirty flags, merge loops into a single walk, and add metrics to ensure pane counts no longer dictate baseline CPU.

### Grid Scrollback Movement
- **Reasons for poor performance:** `grid_move_lines` memmoves large buffers and empties source lines for every scroll event, creating O(height) work per scroll step.
- **Reasons for potential performance improvements:** Representing scrollback as a ring buffer or swapping line pointers reduces operations to constant time and improves cache reuse.
- **Current plans:** Experiment with pointer-swapping line structures, adapt `grid_view_*` helpers to the new model, and stress-test with cat/sustain workloads.

### Hook Dispatch Formatting
- **Reasons for poor performance:** `cmdq_insert_hook` rebuilds argument/flag strings and allocates new command states for each trigger, so hook-heavy setups repeatedly churn heap allocations.
- **Reasons for potential performance improvements:** Caching parsed arguments and providing lightweight hook execution paths trims allocations and string work.
- **Current plans:** Introduce reusable hook format caches keyed by command signature, allow hooks to run with borrowed state, and profile under scripted trigger storms.
