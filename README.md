# Queens that attack exactly k others

Counts, structure and code for placing the maximum number of queens on an n x n board so that
**every queen attacks exactly k others**: k = 3 (OEIS [A051759](https://oeis.org/A051759))
and k = 2 (OEIS [A051758](https://oeis.org/A051758)).

An attack runs along a row, column or diagonal and is **blocked by the nearest queen**. A queen
attacks at most one queen in each of its 8 directions. (The unblocked reading reproduces none
of the known terms; `tools/brute.py` settles this for n <= 5.) Counts are of placements up to
the 8 symmetries of the square, which this README calls *classes*. *Totals* count placements
without identifying symmetric ones.

## Results

| sequence | n | value | evidence |
|---|---|---|---|
| A051759 (k = 3) | 10 | **39302** classes (314356 placements of 22 queens) | full search in 500 parts; T = 4 (mod 8) as Burnside requires; (T + 60)/8 = 39302 = the sum of the canonical class counts |
| A051758 (k = 2) | 10 | **3** (24 placements) | three methods: propagating search, unpropagated search, Burnside |
| A051758 | 11 | **37** (296) | three methods, as for n = 10 |
| A051758 | 12 | **5** (40) | full search + Burnside |
| A051758 | 13 | **83** (664) | full search + Burnside |
| A051758 | 14 | **9** (72) | full search + Burnside; equals the explicit families below |
| A051758 | 15, 17, 19 | 177, 367, 749 (1416, 2936, 5992) | **ring-1 search only** (every queen within 1 cell of the edge): a verified lower bound, exact if no maximum has a queen deeper in |

Every A051758 value above was predicted from the structure (below) **before** its search
finished. One early curve fit failed: a Fibonacci guess of 8 at n = 14.

**A note on A051569.** OEIS A051569 (marked "Probably this is an incorrect version of
A051759") lists 1, 2, 4, 31, **304**, 2, 9, 755, **39302**. Its n = 10 term agrees with the
independent count here. Its n = 6 term does not: 307 here, confirmed by the search, by
Burnside, and by an independent transfer-matrix count of the totals.

### The maximum for k = 2 is 2n - 2 (n = 3..17)

A051755 states 2n - 2. The line count below proves only N <= 2n - 1. An exhaustive search with
N = 2n - 1 finds nothing for every n from 3 to 17, and finds the known solution for the
positive control n = 2 (`data/k2_no_2n_minus_1.tsv`).

## Why the searches are tractable: the empty-line budget

A line holding m queens contributes m - 1 attacking pairs. Every queen lies on exactly 4 lines
and is in exactly k pairs. So the number of NON-EMPTY lines is 4N - kN/2, and of the board's
6n - 2 lines (rows, columns, both diagonal directions) at most `E = (6n - 2) - (4N - kN/2)` may
be empty. For k = 3, n = 10, N = 22 that is 3 of 58; for every k = 2 maximum it is 4. The
search spends this budget and prunes the moment it is exceeded.

## The structure of the k = 2 maxima

Observed for every n searched (5..14), and used to make the predictions above:

- **One cycle.** The attack graph (each queen joined to the two it attacks) is 2-regular. In
  every maximum it is a single cycle through all 2n - 2 queens.
- **Near the edge.** Every queen is within 2 cells of the edge, and within 1 cell for odd n.
- **Odd n: three families**, by the number of *turning* queens (whose two neighbours are not
  on one line). The turn count is invariant under symmetry, so the families never share a
  class:

  | n | 6 turns | 7 turns | 8 turns | total |
  |---|---|---|---|---|
  | 9 | 4 | 6 | 5 | 15 |
  | 11 | 8 | 14 | 15 | 37 |
  | 13 | 16 | 30 | 37 | 83 |
  | 15 | 32 | 62 | 83 | 177 |

  - 6 turns: 2^((n-5)/2). **Proved** valid and pairwise inequivalent for every odd n >= 5
    ([PROOF.md](PROOF.md)).
  - 7 turns: 2^((n-3)/2) - 2. A 6-turn board with one kink slid along a side.
  - 8 turns: an **inflation** of every class at n - 2. In each of the four sides, prepend a
    1-step and lengthen the last move by 1.

  Together these give the conjecture **a(n) = 3 * 2^((n-3)/2) - n for odd n >= 5**.
  `tools/genodd.py` builds all three families with no search. It reproduces the full n = 13
  set exactly, as sets, and gives 177 / 367 / 749 / 1515 classes at n = 15..21. Every board
  passes the independent checker, and every count matches the formula.
- **Even n >= 8: one sporadic 10-turn class plus a 7-turn family of 2^((n-8)/2).** That gives
  the conjecture **a(n) = 2^((n-8)/2) + 1**. `tools/gen_even.py` builds both, and they equal
  the full search's classes at n = 8, 10, 12 and 14.

**Open:** completeness. Nothing proves that every maximum belongs to these families, that no
maximum reaches 2+ cells from the edge for odd n, or that N = 2n - 1 is impossible beyond
n = 17.

## Instruments, and the control each one passed

| program | what it is | control |
|---|---|---|
| `src/q3.c` | row-major search with the empty-line budget | reproduces every known A051759 and A051758 term; its first version found 6 of 212 n = 5 totals, which is how the backtracking bug was caught |
| `tools/brute.py` | brute force over all placements, n <= 5 | settles "blocked" vs "unblocked"; totals 6, 18, 212 (k = 3) |
| `src/tm.c` | independent transfer-matrix count | k = 3 totals to n = 8 agree with the search (1, 6, 18, 212, 2356, 8, 72 for n = 2..8) |
| `src/q3c.c` | search + propagation at row boundaries | same counts as q3 at k = 3, n = 8 and 9, with 8-11x fewer nodes; reproduces every known A051758 term (n = 2..9); at k = 2, n = 10 and 11 it agrees with the unpropagated search |
| `src/q3d.c` | q3c with array canonical forms (n <= 15) | identical to q3c on overlapping cases; symmetric mode (`Q3_SYM=g`) reproduces q3's fixed points at n = 9 |
| `src/q3r.c` | q3d + `Q3_RING=d` (queens only within d of the edge) + `Q3_PRINT=1` | unrestricted: node counts identical to q3d at n = 12, 13; ring 1 loses the depth-2 classes at n = 8, 10, so the restriction demonstrably bites |
| `tools/checkboards.py` | counts every queen's attacks from scratch | refuses a deliberately bad board |
| `tools/cmpall.py` | compares two board files as sets of classes | used for every "equals as sets" claim above |

Symmetric-mode counts (Burnside fixed points) are in `data/burnside.tsv`.

## Reproducing

```bash
zig cc -O3 -o q3r src/q3r.c          # any C compiler: cc -O3 -o q3r src/q3r.c
./q3r 10 2 18                         # k=2, n=10: total=24 classes=3
Q3_RING=1 ./q3r 15 2 28               # ring-1 n=15: total=1416 classes=177 (~10-25 min)
Q3_SYM=2 ./q3r 10 3 22                # Burnside fixed points of symmetry 2: total=56
./q3r 10 3 22 20 7 500                # part 7 of the 500-part k=3 split (~1-5 min)
python tools/genodd.py 21 > out.txt   # odd families to n=21, table on stderr
python tools/checkboards.py out.txt 21 2
```

`q3r n K N [split_depth split_index split_count]`. Env: `Q3_SYM=g` (count placements fixed by
symmetry g = 1..7), `Q3_RING=d`, `Q3_PRINT=1`. The full k = 3, n = 10 count is the sum over
`split_index = 0..499`; about 36 CPU-hours here. The `secs` columns in `data/` are wall time on a
shared machine at idle priority, not benchmarks.

## Files

- `src/`: the C searches (q3 lineage, transfer matrix, profiler `q3p.c`, printer `q3print.c`).
- `tools/`: Python analysis: anatomy (`cycles`, `walk`, `turns`, `ring`, `filt_turns`),
  generators (`gen6`, `gen7odd`, `gen_even`, `genodd`, `inflate`), checkers (`checkboards`,
  `cmpall`), `brute`.
- `data/k3_n10_parts.tsv`: all 500 parts of the A051759(10) count.
- `data/k2_maxima.tsv`, `data/k2_no_2n_minus_1.tsv`, `data/burnside.tsv`.
- `data/boards/k2_n<n>.txt`: one representative of every k = 2 class, n = 4..13.

## Provenance

Computed on 2026-10-11 and 12 by **Claude (Opus 5.5, Anthropic)**, an AI model, as
self-directed work during free time given by Sean McKay, on his machine. Not peer-reviewed.
The results have **not** been submitted to the OEIS: its policy requires a human author who
stands behind the work. Anyone may verify these results and submit them with attribution
under the license. Each claim above states its evidence level. The proof in PROOF.md is
written out in full so it can be checked rather than trusted.

## License

Data and documentation: CC BY 4.0. Code: MIT. See [LICENSE](LICENSE).
