/* $OpenBSD: grid-view.c,v 1.39 2026/08/03 12:58:53 nicm Exp $ */

/*
 * Copyright (c) 2008 Nicholas Marriott <nicholas.marriott@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>

#include <string.h>

#include "tmux.h"

/*
 * Grid view functions. These work using coordinates relative to the visible
 * screen area.
 */

#define grid_view_x(gd, x) (x)
#define grid_view_y(gd, y) ((gd)->hsize + (y))

/* Get cell. */
void
grid_view_get_cell(struct grid *gd, u_int px, u_int py, struct grid_cell *gc)
{
	grid_get_cell(gd, grid_view_x(gd, px), grid_view_y(gd, py), gc);
}

/* Set cell. */
void
grid_view_set_cell(struct grid *gd, u_int px, u_int py,
    const struct grid_cell *gc)
{
	grid_set_cell(gd, grid_view_x(gd, px), grid_view_y(gd, py), gc);
}

/* Set padding. */
void
grid_view_set_padding(struct grid *gd, u_int px, u_int py, int bg)
{
	grid_set_padding(gd, grid_view_x(gd, px), grid_view_y(gd, py), bg);
}

/* Set cells. */
void
grid_view_set_cells(struct grid *gd, u_int px, u_int py,
    const struct grid_cell *gc, const char *s, size_t slen)
{
	grid_set_cells(gd, grid_view_x(gd, px), grid_view_y(gd, py), gc, s,
	    slen);
}

/* Clear into history. */
void
grid_view_clear_history(struct grid *gd, u_int bg)
{
	struct grid_line	*gl;
	u_int			 yy, last;

	/* Find the last used line. */
	last = 0;
	for (yy = 0; yy < gd->sy; yy++) {
		gl = grid_get_line(gd, grid_view_y(gd, yy));
		if (gl->cellused != 0)
			last = yy + 1;
#ifdef ENABLE_IMAGES
		else if (image_grid_line_has_images(gl))
			last = yy + 1;
#endif
	}
	if (last == 0) {
		grid_view_clear(gd, 0, 0, gd->sx, gd->sy, bg);
		return;
	}

	/* Scroll the lines into the history. */
	for (yy = 0; yy < last; yy++) {
		grid_collect_history(gd, 0);
		grid_scroll_history(gd, bg);
	}
	if (last < gd->sy)
		grid_view_clear(gd, 0, 0, gd->sx, gd->sy - last, bg);
	gd->hscrolled = 0;
}

/* Clear area. */
void
grid_view_clear(struct grid *gd, u_int px, u_int py, u_int nx, u_int ny,
    u_int bg)
{
	px = grid_view_x(gd, px);
	py = grid_view_y(gd, py);

	grid_clear(gd, px, py, nx, ny, bg);
}

/* Scroll region up. */
void
grid_view_scroll_region_up(struct grid *gd, u_int rupper, u_int rlower,
    u_int bg)
{
	if (gd->flags & GRID_HISTORY) {
		grid_collect_history(gd, 0);
		if (rupper == 0 && rlower == gd->sy - 1)
			grid_scroll_history(gd, bg);
		else {
			rupper = grid_view_y(gd, rupper);
			rlower = grid_view_y(gd, rlower);
			grid_scroll_history_region(gd, rupper, rlower, bg);
		}
	} else {
		rupper = grid_view_y(gd, rupper);
		rlower = grid_view_y(gd, rlower);
		grid_move_lines(gd, rupper, rupper + 1, rlower - rupper, bg);
	}
}

/* Scroll region down. */
void
grid_view_scroll_region_down(struct grid *gd, u_int rupper, u_int rlower,
    u_int bg)
{
	rupper = grid_view_y(gd, rupper);
	rlower = grid_view_y(gd, rlower);

	grid_move_lines(gd, rupper + 1, rupper, rlower - rupper, bg);
}

/*
 * Scroll a rectangular region bounded by both a vertical [rupper, rlower] and a
 * horizontal [left, right] range. Used when left-right margins are active: only
 * the cells inside the column range move, and the region never scrolls into
 * history (matching xterm, where a margined scroll stays on screen). "up"
 * selects the direction; "n" is the number of lines.
 */
void
grid_view_scroll_pad_region(struct grid *gd, u_int rupper, u_int rlower,
    u_int left, u_int right, int up, u_int n, u_int bg)
{
	struct grid_cell	 gc;
	u_int			 nx, y, x, src;

	if (n == 0 || rupper > rlower || left > right)
		return;
	if (n > rlower - rupper + 1)
		n = rlower - rupper + 1;
	nx = right + 1 - left;

	if (up) {
		/* Move each row up by n, top to bottom. */
		for (y = rupper; y + n <= rlower; y++) {
			src = y + n;
			for (x = left; x <= right; x++) {
				grid_view_get_cell(gd, x, src, &gc);
				grid_view_set_cell(gd, x, y, &gc);
			}
		}
		/* Clear the freed rows at the bottom of the region. */
		grid_view_clear(gd, left, rlower + 1 - n, nx, n, bg);
	} else {
		/* Move each row down by n, bottom to top. */
		for (y = rlower + 1; y-- > rupper + n;) {
			src = y - n;
			for (x = left; x <= right; x++) {
				grid_view_get_cell(gd, x, src, &gc);
				grid_view_set_cell(gd, x, y, &gc);
			}
		}
		/* Clear the freed rows at the top of the region. */
		grid_view_clear(gd, left, rupper, nx, n, bg);
	}
}

/* Insert lines. */
void
grid_view_insert_lines(struct grid *gd, u_int py, u_int ny, u_int bg)
{
	u_int	sy;

	py = grid_view_y(gd, py);

	sy = grid_view_y(gd, gd->sy);

	grid_move_lines(gd, py + ny, py, sy - py - ny, bg);
}

/* Insert lines in region. */
void
grid_view_insert_lines_region(struct grid *gd, u_int rlower, u_int py,
    u_int ny, u_int bg)
{
	u_int	ny2;

	rlower = grid_view_y(gd, rlower);

	py = grid_view_y(gd, py);

	ny2 = rlower + 1 - py - ny;
	grid_move_lines(gd, rlower + 1 - ny2, py, ny2, bg);
	grid_clear(gd, 0, py + ny2, gd->sx, ny - ny2, bg);
}

/* Delete lines. */
void
grid_view_delete_lines(struct grid *gd, u_int py, u_int ny, u_int bg)
{
	u_int	sy;

	py = grid_view_y(gd, py);

	sy = grid_view_y(gd, gd->sy);

	grid_move_lines(gd, py, py + ny, sy - py - ny, bg);
	grid_clear(gd, 0, sy - ny, gd->sx, ny, bg);
}

/* Delete lines inside scroll region. */
void
grid_view_delete_lines_region(struct grid *gd, u_int rlower, u_int py,
    u_int ny, u_int bg)
{
	u_int	ny2;

	rlower = grid_view_y(gd, rlower);

	py = grid_view_y(gd, py);

	ny2 = rlower + 1 - py - ny;
	grid_move_lines(gd, py, py + ny, ny2, bg);
	grid_clear(gd, 0, py + ny2, gd->sx, ny - ny2, bg);
}

/* Insert characters. */
void
grid_view_insert_cells(struct grid *gd, u_int px, u_int py, u_int nx, u_int bg)
{
	u_int	sx;

	px = grid_view_x(gd, px);
	py = grid_view_y(gd, py);

	sx = grid_view_x(gd, gd->sx);

	if (px >= sx - 1)
		grid_clear(gd, px, py, 1, 1, bg);
	else
		grid_move_cells(gd, px + nx, px, py, sx - px - nx, bg);
}

/*
 * Blank a wide-character cell that has been split by a horizontal margin: a
 * leading cell whose padding would fall at or past px, or a padding cell whose
 * leading half is at or before px - 1 (i.e. the split runs across px).
 */
static void
grid_view_clear_split(struct grid *gd, u_int px, u_int py, u_int bg)
{
	struct grid_cell	gc;

	if (px == 0 || px >= gd->sx)
		return;
	grid_get_cell(gd, px, py, &gc);
	if (gc.flags & GRID_FLAG_PADDING) {
		/* Padding whose lead is to the left of the boundary. */
		grid_clear(gd, px - 1, py, 2, 1, bg);
		return;
	}
	grid_get_cell(gd, px - 1, py, &gc);
	if (gc.data.width > 1) {
		/* Wide lead whose padding is to the right of the boundary. */
		grid_clear(gd, px - 1, py, 2, 1, bg);
	}
}

/*
 * Blank a wide-character lead left dangling against a right margin: its cell is
 * at rx - 1 and its padding would have fallen at rx, outside the margin, so
 * only the lead is cleared (rx itself holds protected out-of-margin content).
 */
static void
grid_view_clear_margin_lead(struct grid *gd, u_int rx, u_int py, u_int bg)
{
	struct grid_cell	gc;

	if (rx == 0)
		return;
	grid_get_cell(gd, rx - 1, py, &gc);
	if (gc.data.width > 1)
		grid_clear(gd, rx - 1, py, 1, 1, bg);
}

/*
 * Blank a wide-character orphan exposed at a shift point (px): either a padding
 * cell whose lead was consumed by the shift (clear px), or a lead at px - 1
 * whose padding was overwritten (clear px - 1, but never left of the left
 * margin so out-of-margin content is preserved). Both conditions are checked
 * independently because a single delete can orphan a padding half of one
 * character and the lead half of another.
 *
 * A wide character straddling a margin keeps its out-of-margin half (a lead at
 * left - 1 or padding at the right margin) as a dangling half-cell; this
 * matches plain DCH, which does no wide-character fixup, and self-heals on the
 * next overwrite.
 */
static void
grid_view_clear_shift_point(struct grid *gd, u_int px, u_int left, u_int py,
    u_int bg)
{
	struct grid_cell	gc;

	if (px >= gd->sx)
		return;
	grid_get_cell(gd, px, py, &gc);
	if (gc.flags & GRID_FLAG_PADDING)
		grid_clear(gd, px, py, 1, 1, bg);
	if (px > left) {
		grid_get_cell(gd, px - 1, py, &gc);
		if (gc.data.width > 1)
			grid_clear(gd, px - 1, py, 1, 1, bg);
	}
}

/*
 * Insert characters, bounded on the right by a margin. Cells from px to the
 * right margin shift right by nx; cells that would move past the margin are
 * dropped rather than pushed off the screen.
 */
void
grid_view_insert_cells_right(struct grid *gd, u_int px, u_int py, u_int nx,
    u_int right, u_int bg)
{
	u_int	rx;

	px = grid_view_x(gd, px);
	py = grid_view_y(gd, py);
	rx = grid_view_x(gd, right) + 1;	/* one past the right margin */

	if (px >= rx)
		return;
	if (nx > rx - px)
		nx = rx - px;
	if (px + nx >= rx)
		grid_clear(gd, px, py, rx - px, 1, bg);
	else
		grid_move_cells(gd, px + nx, px, py, rx - px - nx, bg);
	/* Clear the gap opened at the cursor. */
	grid_clear(gd, px, py, nx, 1, bg);
	/*
	 * The shift can leave a wide character split at the end of the moved
	 * run or dangling against the right margin; blank either orphan.
	 */
	grid_view_clear_split(gd, px + nx, py, bg);
	grid_view_clear_margin_lead(gd, rx, py, bg);
}

/* Delete characters. */
void
grid_view_delete_cells(struct grid *gd, u_int px, u_int py, u_int nx, u_int bg)
{
	u_int	sx;

	px = grid_view_x(gd, px);
	py = grid_view_y(gd, py);

	sx = grid_view_x(gd, gd->sx);

	grid_move_cells(gd, px, px + nx, py, sx - px - nx, bg);
	grid_clear(gd, sx - nx, py, nx, 1, bg);
}

/*
 * Delete characters, bounded on the right by a margin. Cells from px+nx to the
 * right margin shift left by nx; the nx cells that open up against the margin
 * are cleared.
 */
void
grid_view_delete_cells_right(struct grid *gd, u_int px, u_int py, u_int nx,
    u_int left, u_int right, u_int bg)
{
	u_int	lx, rx;

	px = grid_view_x(gd, px);
	py = grid_view_y(gd, py);
	lx = grid_view_x(gd, left);
	rx = grid_view_x(gd, right) + 1;	/* one past the right margin */

	if (px >= rx)
		return;
	if (nx > rx - px)
		nx = rx - px;
	grid_move_cells(gd, px, px + nx, py, rx - px - nx, bg);
	grid_clear(gd, rx - nx, py, nx, 1, bg);
	/*
	 * The shift can leave a wide character orphan at the deletion point (a
	 * dangling lead or shifted-in padding) or at the boundary of the
	 * cleared region; blank either without disturbing content outside the
	 * left margin.
	 */
	grid_view_clear_shift_point(gd, px, lx, py, bg);
	grid_view_clear_shift_point(gd, rx - nx, lx, py, bg);
}

/* Convert cells into a string. */
char *
grid_view_string_cells(struct grid *gd, u_int px, u_int py, u_int nx)
{
	px = grid_view_x(gd, px);
	py = grid_view_y(gd, py);

	return (grid_string_cells(gd, px, py, nx, NULL, 0, NULL));
}
