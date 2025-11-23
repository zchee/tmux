#!/bin/sh

# Verify incremental reflow preserves content while skipping untouched tail.

PATH=/bin:/usr/bin
TERM=screen

[ -z "$TEST_TMUX" ] && TEST_TMUX=$(readlink -f ../tmux)
TMUX="$TEST_TMUX -Ltest"
$TMUX kill-server 2>/dev/null

TMP=$(mktemp)
trap "rm -f $TMP" 0 1 15

# Start with narrow width to force wrapping, then widen to trigger reflow.
$TMUX -f/dev/null new -d -x 10 -y 5 "sh -c 'printf \"abcdefghijABCDEFGHIJklm\"; sleep 1'" || exit 1
sleep 0.2
$TMUX resize-window -t 0 -x 15 -y 5 || exit 1

$TMUX capture-pane -t 0 -p -E 1 >$TMP
printf "abcdefghijABCDE\nFGHIJklm\n" | cmp -s $TMP - || exit 1

$TMUX kill-server 2>/dev/null
exit 0
