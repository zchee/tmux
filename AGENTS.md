# tmux Contributor Guide

tmux is a terminal multiplexer written in C that enables multiple terminal sessions within a single screen. This guide helps contributors understand the project structure, development workflow, and coding conventions.

## Project Structure & Module Organization

The project uses a flat structure with all source files in the root directory:

- **Commands**: `cmd-*.c` files implement individual tmux commands
- **Window modes**: `window-*.c` files handle different window modes (copy, buffer, client, etc.)
- **Terminal handling**: `tty-*.c` files manage terminal I/O and features
- **Server logic**: `server-*.c` files contain server and client management
- **Core functionality**: Other `.c` files handle sessions, layouts, grids, input, and options
- **compat/**: OS compatibility layer with platform-specific code (`osdep-*.c`)
- **regress/**: Shell-based regression test suite
- **etc/**: Configuration examples (`example_tmux.conf`)

Key headers: `tmux.h` (main), `compat.h` (compatibility), `tmux-protocol.h` (protocol definitions)

## Build, Test, and Development Commands

**Building from git:**
```bash
sh autogen.sh       # Generate configure script (requires autoconf/automake)
./configure         # Configure build for your platform
make                # Compile tmux
sudo make install   # Install binary and man page
```

**Building from release tarball:**
```bash
./configure && make
sudo make install
```

**Clean rebuild:**
```bash
make clean && sh autogen.sh && ./configure && make
```

**Testing:**
```bash
cd regress && make  # Run regression test suite
```

**Debugging:**
```bash
tmux -v             # Generate server log in current directory
tmux -vv            # Generate both server and client logs
```

## Coding Style & Naming Conventions

- **Indentation**: Use tabs (not spaces)
- **Brace style**: BSD/Allman (opening brace on new line for functions)
- **Naming conventions**:
  - Functions and variables: `snake_case` (e.g., `socket_path`, `find_window`)
  - Macros and constants: `UPPER_CASE` (e.g., `TMUX_CONF`, `PANE_MINIMUM`)
  - Struct names: lowercase with underscores
- **Comments**: C-style block comments `/* */`
- **Line length**: Keep reasonable, typically under 80 characters where practical
- **License header**: All new files must include ISC license header with `/* $OpenBSD$ */` tag

**Compiler standards**: The project uses `-std=gnu99` with strict warnings enabled in debug builds. Ensure code compiles without warnings.

## Testing Guidelines

- **Framework**: Shell-based regression tests in `regress/` directory
- **Test coverage**: Tests cover commands, copy mode, control clients, UTF-8 handling, and more
- **Running tests**: Execute `make` in the `regress/` directory
- **Adding tests**: Create `.sh` scripts following existing patterns; include expected result files where applicable

## Commit & Pull Request Guidelines

**Commit message format:**
- Use imperative mood (e.g., "Add feature", "Fix bug", not "Added" or "Fixes")
- Start with a concise one-line summary
- Reference GitHub issues when applicable: "Fix memory leak, GitHub issue #1234."
- Keep messages focused and descriptive

**Examples from recent commits:**
```
Do not send theme updates to panes that have exited, GitHub issue 4671.
Convert all keys for backspace, not just A-Z.
Add a command to explicitly set the selection mode in copy mode, GitHub issue 3842.
```

**Pull request requirements:**
- Ensure code compiles without warnings
- Run regression tests before submitting
- Include clear description of changes and motivation
- Reference related GitHub issues
- Submit via GitHub pull request or email to tmux-users@googlegroups.com

## Architecture Overview

tmux follows a client-server architecture:
- **Server**: Manages sessions, windows, and panes; runs in background
- **Client**: Connects to server via Unix socket; handles terminal I/O
- **Protocol**: Custom protocol defined in `tmux-protocol.h` for client-server communication

The codebase synchronizes with OpenBSD's version (see `SYNCING` document) and maintains a portable layer in `compat/` for cross-platform support.

## Dependencies

- **Required**: libevent 2.x, ncurses, C compiler (gcc/clang), make, pkg-config, yacc/bison
- **Optional**: utempter (for utmp updates, enable with `--enable-utempter`)
- **Development**: autoconf, automake (for building from git)

## Additional Resources

- **Documentation**: View man page with `nroff -mdoc tmux.1 | less`
- **Wiki**: https://github.com/tmux/tmux/wiki
- **FAQ**: https://github.com/tmux/tmux/wiki/FAQ
- **Mailing list**: https://groups.google.com/forum/#!forum/tmux-users
