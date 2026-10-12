# The 6-turn family of k = 2 placements

**Theorem.** For every odd n >= 5 there are at least 2^((n-5)/2) pairwise inequivalent
placements of 2n - 2 queens on an n x n board in which every queen attacks exactly 2 others
(attacks blocked by the nearest queen). None of them is fixed by any symmetry of the square.

If 2n - 2 is the maximum, this gives A051758(n) >= 2^((n-5)/2) for odd n >= 5. The maximum is
verified by exhaustive search for n <= 17 (`data/k2_no_2n_minus_1.tsv`). For n = 5..13 this
family is exactly the set of 6-turn classes the search finds (checked as sets with
`tools/cmpall.py`).

## Construction

Let n = 2m + 1 with m >= 2. Coordinates are (row, column), 0-based. Let T = {2, ..., n-4}.
For r = 2, ..., m-1 the pairs {r, n-2-r} partition T into m - 2 pairs. When m = 2, T is empty.
Choose S, a subset of T, holding exactly one element of each pair. Let S' = T \ S = {n-2-s : s
in S}, the complementary choice. Place queens in four groups:

| group | line | positions | count |
|---|---|---|---|
| A | column 0 | rows {0, 1, n-1} u S | m + 1 |
| B | row n-2 | columns {1, n-1} u S | m |
| C | column n-2 | rows {0, n-3} u S' | m |
| D | row 0 | columns {0, n-3, n-2} u S' | m + 1 |

A and D share (0, 0); C and D share (0, n-2). Every other pair of groups is disjoint, so there
are 4m = 2n - 2 queens. (`tools/gen6.py` builds exactly these boards from their cycle words.)

## Every queen attacks exactly 2

Throughout, S and S' are disjoint subsets of T = [2, n-4], and n >= 5.

**Rows.** Row 0 holds group D. Row n-2 holds group B. Every other row holds at most one
queen: row 1 holds only (1, 0); row n-3 only (n-3, n-2); row n-1 only (n-1, 0); row r in S
only (r, 0); row r in S' only (r, n-2). The groups' row sets meet only in the shared corners,
because S and S' are disjoint and 1, n-3, n-1 lie outside T.

**Columns.** Similarly, column 0 holds A and column n-2 holds C. Every other column holds at
most one queen: column 1 holds (n-2, 1), column n-1 holds (n-2, n-1), column n-3 holds
(0, n-3), column c in S holds (n-2, c), and column c in S' holds (0, c).

So the attacking pairs along rows and columns are exactly the consecutive queens of four
**chains**: column 0 (A), row 0 (D), row n-2 (B) and column n-2 (C). A queen inside a chain
attacks its two chain neighbours. The chain ends are:

- (0, 0): an end of both column 0 and row 0, so 2 attacks.
- (0, n-2): an end of both row 0 and column n-2, so 2 attacks.
- (n-1, 0), (n-2, 1), (n-2, n-1), (n-3, n-2): each the end of exactly one chain, so 1 attack
  so far.

**Diagonals.** Index the anti-diagonals by r + c and the main diagonals by r - c.

| group | r + c | r - c |
|---|---|---|
| A | {0, 1, n-1} u S | {0, 1, n-1} u S |
| B | {n-1, 2n-3} u (n-2+S) | {n-3, -1} u S' |
| C | {n-2, 2n-5} u (n-2+S') | {-(n-2), -1} u (-S) |
| D | {0, n-3, n-2} u S' | {0, -(n-3), -(n-2)} u (-S') |

(For B: (n-2) - c with c in S is n-2-S = S'. For C: r - (n-2) with r in S' is -(n-2-S') = -S.)

Within one group the values are distinct. Across groups, the only common values are:

- r + c = 0, r - c = 0: (0, 0), a single queen in A and D.
- r + c = n-2, r - c = -(n-2): (0, n-2), a single queen in C and D.
- **r + c = n-1:** (n-1, 0) in A and (n-2, 1) in B. Adjacent cells.
- **r - c = -1:** (n-2, n-1) in B and (n-3, n-2) in C. Adjacent cells.

Every other coincidence is excluded by the ranges. S, S' lie in [2, n-4], so n-2+S and n-2+S'
lie in [n, 2n-6]. Also 2n-5 >= n and 2n-3 > 2n-6 for n >= 5, and S n S' is empty. So exactly
two diagonal attacks exist, each between adjacent cells. Each gives the second attack to the
two chain ends that had only one. **Every queen attacks exactly 2.**

## Inequivalence and trivial stabilisers

Lines map to lines under the symmetries of the square. Row 0 and column 0 are the only lines
holding m + 1 queens: rows n-2 and column n-2 hold m, other rows and columns at most 1, and
diagonals at most 2. So a symmetry g taking board(S1) to board(S2) maps {row 0, column 0} to
itself, and fixes their common cell (0, 0). That leaves the identity and the transpose
(r, c) -> (c, r).

The transpose would map row 0 of board(S1), with columns {0, n-3, n-2} u S1', onto column 0
of board(S2). But column 0 always contains row 1, and 1 is not in {0, n-3, n-2} u S1' for
n >= 5. So g is the identity and S1 = S2. The 2^(m-2) boards are pairwise inequivalent, and
(taking S1 = S2) none is fixed by any non-trivial symmetry. Each class contributes 8
placements.

## Numerical receipts

- `python tools/gen6.py N > f.txt` then `python tools/checkboards.py f.txt N 2`: 0 bad
  boards for N = 5..31 (8192 classes at N = 31).
- As sets, the family equals the exhaustive search's 6-turn classes at n = 5, 7, 9, 11 and 13.
