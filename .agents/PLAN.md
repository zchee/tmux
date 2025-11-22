## Command Queue Drain Plan (2025-11-17)
1. [x] Restate the objective plus AGENTS constraints so the command-queue work stays compliant.
2. [x] Capture current git status for a baseline.
3. [x] Re-read the server loop (`server.c:server_loop`) to understand the existing busy-drain pattern.
4. [x] Review `cmdq.c` (structures, append/remove paths) to see where we can hook counters.
5. [x] Inspect all places where `CMDQ_WAITING` is set/cleared (e.g., `cmdq_next`, `cmdq_continue`) so a readiness metric stays accurate.
6. [x] Design a readiness counter (or similar) that tracks runnable items instead of blindly scanning every loop.
7. [x] Update `struct cmdq_item` helpers (append/insert/remove) to maintain the readiness counter when items enter/leave queues.
8. [x] Adjust the WAITING transitions (`cmdq_next`, `cmdq_continue`) to decrement/increment the counter when items start/stop waiting.
9. [x] Teach `server_loop` to skip the expensive drain unless the readiness counter is non-zero, while still draining until the counter drops back to zero.
10. [x] Double-check any other code paths that manipulate CMDQ_WAITING or queues (hooks, job callbacks) so the counter never drifts.
11. [x] Rebuild the affected objects (`make server.o cmd-queue.o`) to ensure compilation succeeds.
12. [x] Outline a validation approach (e.g., tracing ready-count transitions or perf counters) since there isn’t an automated test.
13. [x] Update this PLAN.md with findings/results once implementation finishes.
14. [x] Summarize the change and its trade-offs for the user.
15. [x] Log follow-up ideas (e.g., event-driven wakeups or metrics) for future work.

### Findings Log
- Added a per-queue readiness tracker so `server_loop` only drains when at least one queue head is runnable, eliminating the previous do/while busy scan at idle.
- Rebuilt `tmux` plus the `regress` suite with the readiness changes and fixed the misplaced `cmdq_ready` prototype so it remains inside the header guard.

### Measurement / Validation Ideas
- Temporarily log `cmdq_ready_queues` transitions and sample `perf top` while tmux is idle to confirm CPU drops to zero.
- Script a workload where `run-shell` jobs wait on timers and verify (via logs) that server_loop sleeps until the waiter resumes.

### Follow-up Ideas
- Promote the readiness counter to metrics (e.g., `tmux display-message #{cmdq_ready}`) for easier introspection.
- Explore integrating libevent timers so a newly ready queue can wake the loop immediately rather than waiting for the next event tick.

## Performance Recon (2025-11-21)
- Status line work still recreates a full `format_tree` and re-expands every `status-format` line on each timer tick even when the rendered text is unchanged (`status.c` around `status_redraw`). We could cache per-format expansions (or a compiled AST) keyed by the format string and a dependency mask (time/job/content) and skip both expansion and draw when inputs are unchanged.
- Pane border status redraw (`screen_redraw_make_pane_status` in `screen-redraw.c`) always builds a new format tree, expands the format, reinitializes a 1-line screen, and draws it for every visible pane, then compares grids afterward. Memoizing the expanded string + width per pane and short-circuiting when unchanged would drop per-redraw cost in multi-pane layouts.
- `format_find` (format.c ~3926) walks up to six option trees and hits `format_table` for every key lookup with no per-tree memoization. Adding a simple per-`format_tree` cache of resolved keys (including option hits) would shrink repeated lookups inside a single expansion (hot for status/pane formats with repeated keys and nested expansions).

### 2025-11-21 Changes
- Implemented a per-`format_tree` lookup cache in `format_find`, covering option/env/table hits and time conversions; tree alloc/frees now manage cached entries. Expect reduced CPU in deep format expansions (status lines, pane borders, menu drawing) without behavior changes.
- Local `make format.o ...` initially failed due to missing toolchain path (`/Applications/Xcode-26.2.0-Beta.2.app/.../clang`); reran successfully after toolchain path was fixed.

### 2025-11-21 Pane Border Cache
- Added per-pane cache of the expanded `pane-border-format`, derived width, and applied grid cell. If width, content, and style match the previous values, we skip rebuilding the pane status screen and return early. Cached state is allocated in `window.c` and freed on pane destroy. This reduces per-redraw work when pane-border-status is enabled and unchanged.
- Status line per-line cache: each `status-format` entry now remembers the originating format pointer and the expansion time, skipping re-expansion within the same server tick when nothing changed. This trims repeated format parsing when multiple redraws happen in a short window.
- Theme update sweep shortcut: before walking every pane each loop, we now test if any pane has `PANE_THEMECHANGED`; if none, we skip the theme update pass (server-client.c). Reduces pointless per-loop pane scans in steady state.

### 2025-11-21 Static Status Timer
- Added `format_has_time` tracking to flag status formats that expand time or time-like fields. Status redraw now records whether any line needed time; the status timer only reschedules when time is present, eliminating the default 1 Hz wakeups for purely static formats.
- Safe fallback: timers restart on option changes or capability changes via existing `status_timer_start*` call sites, so switching to a time-bearing format re-enables periodic redraws automatically.

### 2025-11-21 Format/Name Fast Paths
- format.c now short-circuits expansion when strings contain no `#` placeholders (after `strftime`), skipping the parser entirely for static formats.
- Automatic rename is now suppressed for windows not visible in any attached session, eliminating background rename work on idle/offscreen windows; rename resumes automatically when a window becomes visible.

## Future Performance Roadmap (rolling)
- Short-circuit status message/prompt redraws: avoid full screen copies when only the prompt buffer changes by overwriting the relevant line.
- Defer automatic-rename formatting when a window is not visible in any attached client; refresh when it becomes visible. Prevents background rename churn on noisy panes.
- Investigate adaptive status timer backoff: lengthen status timer when no flagged changes occur; reset on any dependency change.

## New Backlog (2025-11-21)
- Incremental reflow: only rewrap lines that overflow/underflow on resize instead of walking full history; keep per-line wrap metadata to bound work to changed rows. Risks: WRAPPED flag consistency, history scroll offsets.
- Status prompt/message short-circuit: redraw just the prompt line instead of screen_copy of the entire status buffer when only prompt text changes. Need to ensure cursor style/colour updates still apply.
- Shared format AST + dependency mask: pre-parse format strings once, reuse across redraws, and track dynamic tokens (time/jobs/options) to skip parsing and known-stable subtrees.

### Recent Micro-Optimizations (for context)
- Skipped status timer when formats are static (format_has_time + status_has_time).
- Fast-path format expansion when no `#` placeholders after strftime.
- Suppressed automatic rename for windows invisible to attached clients.
- Reflow guards for unchanged width and for cases where all lines already fit the new width.

### 2025-11-21 Q&A Notes
- Next step for the Dramatic Performance Improvement Backlog: start with the first item under “New Backlog (2025-11-21)” — the incremental reflow optimization (rewrap only overflow/underflow lines on resize).

### 2025-11-21 Incremental Reflow Work
- Added cached line-width metadata (`GRID_LINE_WIDTHVALID` + `cached_width`) so reflow checks reuse computed widths instead of rescanning extended cells every resize; caches are invalidated on line edits.
- grid_reflow now skips untouched leading lines and measures line width via the cache helper, then rewraps only from the first line that overflows/was wrapped; updates `gd->sx` on completion.

### 2025-11-22 Prompt/Message Short-Circuit
- status_message_redraw/status_prompt_redraw now reuse the existing active status screen when size is unchanged and only copy the prompt/message line from the base screen before painting text, avoiding a full screen_copy on every keystroke while keeping cursor colour/style updates intact.
