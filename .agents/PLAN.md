## Status Job Subscriber Plan (2025-11-17)
1. [x] Restate instructions/constraints (AGENTS.md) so the subscriber work stays compliant.
2. [x] Record baseline git status.
3. [x] Summarize the current context-aware job cache to ensure we understand the starting point.
4. [x] Design the subscriber model: what data structure tracks interested clients per job, and how do we notify them?
5. [x] Update the plan for `struct format_job` to store subscriber lists/reference counts.
6. [x] Decide what key uniquely identifies a subscription (client pointer, state snapshot) and how to remove it when the client dies.
7. [x] Modify `format_job_get` to attach the calling client as a subscriber when it reuses an existing job.
8. [x] Ensure the code adds new subscribers when jobs are first created and avoids duplicates for the same client.
9. [x] Wire job update/completion callbacks so every subscriber gets `server_status_client` notifications when output changes.
10. [x] Implement cleanup: removing subscribers when a client exits or when the job is tidied.
11. [x] Update `format_lost_client` (and any other teardown paths) to unsubscribe the departing client.
12. [x] Rebuild `format.o` (and any dependent objects) to validate compilation.
13. [x] Sketch a validation approach (e.g., logging subscriber counts) since automated tests do not cover this path.
14. [x] Record findings and follow-up ideas in this PLAN.md once implementation is complete.
15. [x] Summarize the code changes and trade-offs for the user.

### Findings Log
- Format jobs are now keyed by the expanded command string plus cwd. Jobs keep subscriber lists so identical commands from multiple clients share a single background process and receive shared notifications.

### Measurement / Validation Ideas
- Add temporary logging of subscriber counts and `job_run` invocations while running a multi-client status setup to ensure sharing activates.
- Use `tmux -vv` logs to confirm that identical status formats no longer spawn duplicate jobs per client.

### Follow-up Ideas
- Expose subscriber/cache hit statistics via an option or `display-message` hook so operators can see sharing effectiveness.
- Consider evicting unused jobs sooner once their subscriber list becomes empty.
