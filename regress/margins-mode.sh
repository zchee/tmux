#!/bin/sh

# Test DECLRMM (private mode ?69) and DECSLRM (CSI Pl;Pr s) left/right
# margins, plus the CSI s SCP/DECSLRM disambiguation.
#
# DECRQM (ESC[?69$p) should elicit DECRPM (ESC[?69;Ps$y) where Ps=1 when
# MODE_LEFT_RIGHT_MARGIN is active and Ps=2 when reset. When ?69 is reset,
# CSI s is SCP (save cursor); when ?69 is set, CSI Pl;Pr s sets margins.

PATH=/bin:/usr/bin
TERM=screen

[ -z "$TEST_TMUX" ] && TEST_TMUX=$(readlink -f ../tmux)
TMUX="$TEST_TMUX -Ltest69A$$ -f/dev/null"
$TMUX kill-server 2>/dev/null
sleep 1

TMP=$(mktemp)
trap "rm -f $TMP; $TMUX kill-server 2>/dev/null" 0 1 15

$TMUX -f/dev/null new -d -x80 -y24 || exit 1
sleep 1

$TMUX set -g remain-on-exit on

exit_status=0

# query_decrpm <outfile> <mode> [setup_seq] <count>
query_decrpm() {
	_outfile=$1
	_mode=$2
	_setup=$3
	_n=$4

	$TMUX respawnw -k -t:0 -- sh -c "
		exec 2>/dev/null
		stty raw -echo
		${_setup:+printf '$_setup'; sleep 0.2}
		printf '\033[%s\$p' "$_mode"
		dd bs=1 count=$_n 2>/dev/null | cat -v > $_outfile
		sleep 0.2
	" || exit 1
	sleep 2
}

check() {
	_label=$1
	_expected=$2
	_actual=$(cat "$TMP")
	if [ "$_actual" = "$_expected" ]; then
		[ -n "$VERBOSE" ] && echo "[PASS] $_label -> $_actual"
	else
		echo "[FAIL] $_label: expected '$_expected', got '$_actual'"
		exit_status=1
	fi
}

# The DECRPM reply "ESC[?69;Ps$y" is 9 bytes.
# ------------------------------------------------------------------
# Test 1: mode 69 reset by default (Ps=2)
# ------------------------------------------------------------------
query_decrpm "$TMP" "?69" '' 9
check "DECRQM 69 (default/reset)" '^[[?69;2$y'

# ------------------------------------------------------------------
# Test 2: DECSET ?69, then query (expect Ps=1)
# ------------------------------------------------------------------
query_decrpm "$TMP" "?69" '\033[?69h' 9
check "DECRQM 69 (set)" '^[[?69;1$y'

# ------------------------------------------------------------------
# Test 3: DECRST ?69, then query (expect Ps=2)
# ------------------------------------------------------------------
query_decrpm "$TMP" "?69" '\033[?69l' 9
check "DECRQM 69 (reset)" '^[[?69;2$y'

# ------------------------------------------------------------------
# Test 4: SCP preserved when ?69 clear. Save cursor at row1 col6, move to
# row1 col1, restore, then DSR 6n reports the restored position (1;6).
# ------------------------------------------------------------------
$TMUX respawnw -k -t:0 -- sh -c "
	exec 2>/dev/null
	stty raw -echo
	printf '\033[1;6H\0337\033[1;1H\0338\033[6n'
	dd bs=1 count=6 2>/dev/null | cat -v > $TMP
	sleep 0.2
" || exit 1
sleep 2
check "SCP preserved (mode clear)" '^[[1;6R'

# ------------------------------------------------------------------
# Test 5: DECSLRM active still leaves DECRQM ?69 set; bare CSI s (margin
# reset) and CSI Pl;Pr s must not disturb the mode bit.
# ------------------------------------------------------------------
query_decrpm "$TMP" "?69" '\033[?69h\033[5;20s\033[s' 9
check "DECRQM 69 (set, after DECSLRM + bare s)" '^[[?69;1$y'

# check_capture <label> <printf-seq> <awk-condition-desc> <expected-line-2>
# Runs a pane that emits the sequence, captures the top rows, and compares a
# specific captured row (0-indexed) to an expected string.
capture_row() {
	_label=$1; _seq=$2; _row=$3; _expected=$4
	# Keep the pane alive while capturing; a pane that exits clears its
	# screen before capture-pane can read it.
	$TMUX respawnw -k -t:0 -- sh -c "
		exec 2>/dev/null
		stty raw -echo
		printf '$_seq'
		sleep 10
	" || exit 1
	sleep 1.5
	_actual=$($TMUX capturep -p -t:0 | sed -n "$((_row + 1))p")
	if [ "$_actual" = "$_expected" ]; then
		[ -n "$VERBOSE" ] && echo "[PASS] $_label -> '$_actual'"
	else
		echo "[FAIL] $_label: expected '$_expected', got '$_actual'"
		exit_status=1
	fi
}

# ------------------------------------------------------------------
# Test 6: autowrap to left margin. Margins columns 3..8 (1-based), cursor to
# the left margin, print 10 letters. With wrapping they occupy [3..8] on row
# 1 then wrap to [3..8] on row 2 (col 1-2 stay blank).
#   ESC[2J clear, ESC[H home, ESC[?69h, ESC[3;8s margins, ESC[1;3H to (r1,c3)
# Row 0 should be "  ABCDEF" (2 leading spaces, then 6 chars to col 8).
# ------------------------------------------------------------------
capture_row "wrap within margins (row0)" \
	'\033[2J\033[H\033[?69h\033[3;8s\033[1;3HABCDEFGHIJ' \
	0 '  ABCDEF'

# Row 1 should be "  GHIJ" (wrapped remainder starting at left margin col 3).
capture_row "wrap within margins (row1)" \
	'\033[2J\033[H\033[?69h\033[3;8s\033[1;3HABCDEFGHIJ' \
	1 '  GHIJ'

# ------------------------------------------------------------------
# Test 7: carriage return returns to the left margin. After setting margins
# 3..8 and printing from col 3, a CR then 'Z' overwrites the left margin cell.
#   Print ABC (cols 3,4,5), CR, Z -> col 3 becomes Z: "  ZBC"
# ------------------------------------------------------------------
capture_row "CR to left margin" \
	'\033[2J\033[H\033[?69h\033[3;8s\033[1;3HABC\rZ' \
	0 '  ZBC'

# ------------------------------------------------------------------
# Test 8: bounded editing operations. Margins are columns 3..8 (1-based),
# i.e. left margin index 2 and right margin index 7. Each op must leave the
# columns OUTSIDE [3..8] untouched while the inside scrolls or shifts.
#
# Shared fill (before enabling margins so the outer columns are painted):
#   row1 aaaaaaaaaaaa, row2 bbbbbbbbbbbb, row3 cccccccccccc, row4 dddddddddddd
# Then ESC[?69h ESC[3;8s enables margins 3..8.
# ------------------------------------------------------------------
FILL='\033[2J\033[Haaaaaaaaaaaa\r\nbbbbbbbbbbbb\r\ncccccccccccc\r\ndddddddddddd\033[?69h\033[3;8s'

# DL 1 at row2 col3: the sub-rectangle [row2..bottom]x[cols3..8] scrolls up.
# Row2 outer columns "bb....bbbb" stay; inner becomes row3's inner "cccccc".
capture_row "DL within margins keeps outer (row1)" \
	"$FILL"'\033[2;3H\033[M' 0 'aaaaaaaaaaaa'
capture_row "DL within margins scrolls inner (row2)" \
	"$FILL"'\033[2;3H\033[M' 1 'bbccccccbbbb'
capture_row "DL within margins pads bottom row inner" \
	"$FILL"'\033[2;3H\033[M' 2 'ccddddddcccc'

# IL 1 at row2 col3: the sub-rectangle scrolls down; row2 inner is blanked,
# outer columns untouched.
capture_row "IL within margins blanks inner (row2)" \
	"$FILL"'\033[2;3H\033[L' 1 'bb      bbbb'
capture_row "IL within margins shifts inner down (row3)" \
	"$FILL"'\033[2;3H\033[L' 2 'ccbbbbbbcccc'

# IL at row2 col1 (col1 is OUTSIDE the margins) must be a complete no-op.
capture_row "IL outside margins is a no-op (row2)" \
	"$FILL"'\033[2;1H\033[L' 1 'bbbbbbbbbbbb'

# ICH 3 at row1 col4 confined by the right margin: "ABCDEF" occupies cols
# 3..8; inserting 3 blanks at col4 keeps 'A' at col3, pushes 'BC' to cols
# 7..8, and drops "DEF" past the right margin.
capture_row "ICH bounded by right margin" \
	'\033[2J\033[H\033[?69h\033[3;8s\033[1;3HABCDEF\033[1;4H\033[3@' \
	0 '  A   BC'

# DCH 2 at row1 col4 confined by the right margin: deleting 'BC' pulls "DEF"
# left, leaving "ADEF" at cols 3..6 and blanking cols 7..8.
capture_row "DCH bounded by right margin" \
	'\033[2J\033[H\033[?69h\033[3;8s\033[1;3HABCDEF\033[1;4H\033[2P' \
	0 '  ADEF'

# cursor_after <label> <printf-seq> <expected-cursor_x>
# Runs a pane, then reports the pane cursor X (0-based) via display -p.
cursor_after() {
	_label=$1; _seq=$2; _expected=$3
	$TMUX respawnw -k -t:0 -- sh -c "
		exec 2>/dev/null
		stty raw -echo
		printf '$_seq'
		sleep 10
	" || exit 1
	sleep 1.5
	_actual=$($TMUX display -p -t:0 '#{cursor_x}')
	if [ "$_actual" = "$_expected" ]; then
		[ -n "$VERBOSE" ] && echo "[PASS] $_label -> cursor_x=$_actual"
	else
		echo "[FAIL] $_label: expected cursor_x=$_expected, got '$_actual'"
		exit_status=1
	fi
}

# ------------------------------------------------------------------
# Test 9: HT (tab) stops at the right margin. Margins 3..6 (0-based right=5).
# From the left margin the next default tab stop (0-based col 8) is past the
# right margin, so the tab must clamp the cursor to the right margin (col 5).
# ------------------------------------------------------------------
cursor_after "HT clamps to right margin" \
	'\033[2J\033[H\033[?69h\033[3;6s\033[1;3H\t' \
	5

# ------------------------------------------------------------------
# Test 10: CBT (back-tab) does not cross the left margin. Margins 3..8
# (0-based left=2). From col 8 with no tab stops inside the region, back-tab
# must clamp the cursor to the left margin (0-based col 2).
# ------------------------------------------------------------------
cursor_after "CBT clamps to left margin" \
	'\033[2J\033[H\033[?69h\033[3;8s\033[1;8H\033[Z' \
	2

# ------------------------------------------------------------------
# Test 11: RIS (ESC c) clears the mode. Set ?69, then RIS, then DECRQM -> ;2.
# ------------------------------------------------------------------
query_decrpm "$TMP" "?69" '\033[?69h\033c' 9
check "DECRQM 69 (reset by RIS)" '^[[?69;2$y'

# ------------------------------------------------------------------
# Test 12: DECALN (ESC # 8) resets margins. Set ?69 and margins, DECALN fills
# the 80-column screen with E and resets margins to full width; writing 'ZZZ'
# from home lands at column 1 (not the old left margin), proving the reset.
# ------------------------------------------------------------------
DECALN_EXPECTED=ZZZ$(awk 'BEGIN{for(i=0;i<77;i++)printf "E"}')
capture_row "DECALN resets margins (write from col1)" \
	'\033[?69h\033[3;8s\033#8\033[HZZZ' \
	0 "$DECALN_EXPECTED"

# ------------------------------------------------------------------
# Test 13: resizing the window while margins are active must not leave the
# right margin pointing past the (smaller) grid. Set margins near the right
# edge, shrink the width below them, and confirm the server survives and the
# session still responds (no out-of-bounds crash).
# ------------------------------------------------------------------
$TMUX set -g window-size manual
$TMUX respawnw -k -t:0 -- sh -c "
	exec 2>/dev/null
	stty raw -echo
	printf '\033[?69h\033[60;78s'
	sleep 10
" || exit 1
sleep 1
$TMUX resizew -x 20 2>/dev/null
$TMUX resizew -x 80 2>/dev/null
sleep 0.5
if $TMUX has-session 2>/dev/null; then
	[ -n "$VERBOSE" ] && echo "[PASS] resize with margins survives (no OOB)"
else
	echo "[FAIL] resize with margins: server died (likely OOB)"
	exit_status=1
fi

# ------------------------------------------------------------------
# Test 14: a wide character shifted against the right margin must not leave a
# dangling half, and the out-of-margin column must be preserved. Margins 1..6
# (0-based right=5). Put Y at column 7 (outside), a wide char at columns 5-6
# (the wide char occupies the right margin edge), then DCH at column 1. The
# wide char stays whole inside the margins and Y at column 7 is untouched.
# ------------------------------------------------------------------
$TMUX respawnw -k -t:0 -- sh -c "
	exec 2>/dev/null
	stty raw -echo
	printf '\033[2J\033[H\033[1;7HY\033[?69h\033[1;6s\033[1;5H\344\275\240\033[1;1H\033[P'
	sleep 10
" || exit 1
sleep 1.5
widerow=$($TMUX capturep -p -t:0 | sed -n 1p)
# Expect: three leading spaces, the wide char, a space, then Y at column 7.
if [ "$widerow" = "   $(printf '\344\275\240') Y" ]; then
	[ -n "$VERBOSE" ] && echo "[PASS] wide char at right margin -> '$widerow'"
else
	echo "[FAIL] wide char at right margin: got '$widerow'"
	exit_status=1
fi

# capture_row already keeps the pane alive; reuse it for the DCH wide-char cases.

# ------------------------------------------------------------------
# Test 15: DCH that splits a wide character at the deletion range end must not
# destroy the innocent cell before it. Margins 1..10, row "ABCDEfg<W>z", cursor
# at column 6, DCH 3: the wide char's remnant shifts left but 'E' at column 5
# must survive. Expect "ABCDE z".
# ------------------------------------------------------------------
capture_row "DCH wide-split keeps prior cell" \
	'\033[2J\033[H\033[?69h\033[1;10s\033[1;1HABCDEfg\344\275\240z\033[1;6H\033[3P' \
	0 'ABCDE z'

# ------------------------------------------------------------------
# Test 16: DCH at the left margin that exposes a wide-character orphan must not
# corrupt the column left of the left margin. Margins 3..10, "AB" at columns
# 1..2 (outside), "C<W>xyz" from column 3, cursor at the left margin (col 3),
# DCH 2: 'B' at column 2 (outside the margin) must be preserved. Expect
# "AB xyz".
# ------------------------------------------------------------------
capture_row "DCH at left margin keeps out-of-margin cell" \
	'\033[2J\033[H\033[?69h\033[3;10s\033[1;1HAB\033[1;3HC\344\275\240xyz\033[1;3H\033[2P' \
	0 'AB xyz'

# ------------------------------------------------------------------
# Test 17: DCH deleting a run that both starts on one wide char's padding and
# ends on another's lead must blank both orphans. Margins 1..20, row "A<W1><W2>xy"
# (W1 cols 2-3, W2 cols 4-5), cursor on W1's trailing half (col 3), DCH 2 removes
# W1's padding and W2's lead. Both wide chars are destroyed. Expect "A  xy".
# ------------------------------------------------------------------
capture_row "DCH clears both compound wide orphans" \
	'\033[2J\033[H\033[?69h\033[1;20s\033[1;1HA\344\275\240\345\245\275xy\033[1;3H\033[2P' \
	0 'A  xy'

# ------------------------------------------------------------------
# Test 18: DCH spanning the full margin width from the left margin must not
# clear a wide character left of the left margin. "A<W>xyz" (W at cols 2-3)
# printed before margins, then margins 3..10, cursor at the left margin (col 3,
# W's trailing half), DCH 8 (full margin width). W at columns 2-3 is outside the
# left margin and must be preserved. Expect "A<W>".
# ------------------------------------------------------------------
capture_row "DCH full-width from left margin keeps outside wide char" \
	'\033[2J\033[H\033[1;1HA\344\275\240xyz\033[?69h\033[3;10s\033[1;3H\033[8P' \
	0 "A$(printf '\344\275\240')"

$TMUX kill-server 2>/dev/null

exit $exit_status
