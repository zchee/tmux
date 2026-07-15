#!/bin/sh

# Test DECSET 2027 (grapheme cluster processing): DECRQM reporting, reset on
# RIS and VS16 forcing the previous character wide while the mode is set.
# Only behaviour independent of utf8proc is asserted so the result is the
# same whether or not tmux is built with it.

PATH=/bin:/usr/bin
TERM=screen

[ -z "$TEST_TMUX" ] && TEST_TMUX=$(readlink -f ../tmux)
TMUX="$TEST_TMUX -Ltest2027$$ -f/dev/null"
$TMUX kill-server 2>/dev/null

SCRIPT=$(mktemp)
OUT=$(mktemp)
trap "rm -f $SCRIPT $OUT" 0 1 15

cat >$SCRIPT <<'EOF'
OUT="$1"
stty raw -echo min 0 time 10
q() {
	printf '\033[?2027$p'
	printf '%s%s\n' "$1" "$(dd bs=64 count=1 2>/dev/null)" | cat -v >>"$OUT"
}
c() {
	printf '\033[6n'
	printf '%s%s\n' "$1" "$(dd bs=64 count=1 2>/dev/null)" | cat -v >>"$OUT"
}
q A
printf '\033[?2027h'
q B
printf '\033[?2027l'
q C
printf '\033[?2027h'
printf '\033c'
q D
printf '\033[?2027l'
printf '\r\033[K\342\235\244\357\270\217'
c E
printf '\033[?2027h'
printf '\r\033[K\342\235\244\357\270\217'
c F
EOF

$TMUX new -d 'sleep 60' || exit 1
$TMUX set -g variation-selector-always-wide off
$TMUX neww -d "sh $SCRIPT $OUT"

n=0
while [ "$(wc -l <$OUT)" -lt 6 ] && [ $n -lt 15 ]; do
	sleep 1
	n=$((n + 1))
done
$TMUX kill-server 2>/dev/null

cmp $OUT grapheme-cluster-mode.result || exit 1

exit 0
