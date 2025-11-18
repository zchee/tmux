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
