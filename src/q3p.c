/* q3.c -- count placements of N queens on an n x n board in which every queen attacks
 * exactly K others (attacks are along rows, columns and both diagonals, blocked by the
 * nearest queen), up to the 8 symmetries of the square.  OEIS A051759 is K = 3 with N the
 * maximum A051756(n).
 *
 * The pruning that makes this tractable: a line holding m queens gives m-1 attacking
 * pairs, every queen is on exactly 4 lines and is in exactly K pairs, so
 *     sum over non-empty lines (m-1) = K*N/2   and   sum m = 4N
 * hence the number of non-empty lines is 4N - K*N/2, and at most
 *     E = (6n - 2) - (4N - K*N/2)
 * of the board's 6n-2 lines may be empty.  For K = 3, n = 10, N = 22 that is 3 of 58.
 *
 * usage: q3 n K N [split_depth split_index split_count]
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAXN 16
static int n, K, NQ;
static int budget;                    /* empty lines still allowed */
static int qr[MAXN * MAXN], qc[MAXN * MAXN], cnt[MAXN * MAXN];
static int nq;
static int openRow, openCol[MAXN], openD1[2 * MAXN], openD2[2 * MAXN];
static int lcRow[MAXN], lcCol[MAXN], lcD1[2 * MAXN], lcD2[2 * MAXN];  /* queens on line */
static unsigned char board[MAXN][MAXN];
static long long total, classes;
static long long nodes;
static long long rowNodes[MAXN + 1], kills[8];

/* last cell (row-major index) of the d1 (down-right) and d2 (down-left) line through (r,c) */
static int lastD1(int r, int c) { int k = (n - 1 - r) < (n - 1 - c) ? (n - 1 - r) : (n - 1 - c); return (r + k) * n + (c + k); }
static int lastD2(int r, int c) { int k = (n - 1 - r) < c ? (n - 1 - r) : c; return (r + k) * n + (c - k); }

/* can queen i still reach K?  `pos` is the next undecided cell. */
static int feasible(int i, int pos) {
    int r = qr[i], c = qc[i], pot = 0;
    if (openRow == i && r * n + (n - 1) >= pos && r == pos / n) pot++;
    if (openCol[c] == i && (n - 1) * n + c >= pos) pot++;
    if (openD1[r - c + n - 1] == i && lastD1(r, c) >= pos && lastD1(r, c) != r * n + c) pot++;
    if (openD2[r + c] == i && lastD2(r, c) >= pos && lastD2(r, c) != r * n + c) pot++;
    return cnt[i] + pot >= K;
}

/* ---- symmetry: 8 images of the board as 128-bit masks; count canonical solutions ---- */
typedef unsigned __int128 u128;
static u128 image(int t) {
    u128 m = 0;
    for (int r = 0; r < n; r++) for (int c = 0; c < n; c++) if (board[r][c]) {
        int rr, cc;
        switch (t) {
            case 0: rr = r; cc = c; break;
            case 1: rr = c; cc = n - 1 - r; break;
            case 2: rr = n - 1 - r; cc = n - 1 - c; break;
            case 3: rr = n - 1 - c; cc = r; break;
            case 4: rr = r; cc = n - 1 - c; break;
            case 5: rr = n - 1 - r; cc = c; break;
            case 6: rr = c; cc = r; break;
            default: rr = n - 1 - c; cc = n - 1 - r; break;
        }
        m |= (u128)1 << (rr * n + cc);
    }
    return m;
}
static void record(void) {
    u128 me = image(0);
    int smaller = 0;
    for (int t = 1; t < 8; t++) if (image(t) < me) { smaller = 1; break; }
    total++;
    if (!smaller) classes++;
}

/* Symmetric mode (for Burnside): count only placements fixed by symmetry `symg` (1..7, the
 * same numbering as image()).  A cell whose image under g was already decided is FORCED to
 * that decision; at the end the whole board is checked against its image. */
static int symg = 0;
static int symImage(int t, int r, int c) {
    switch (t) {
        case 1: return c * n + (n - 1 - r);
        case 2: return (n - 1 - r) * n + (n - 1 - c);
        case 3: return (n - 1 - c) * n + r;
        case 4: return r * n + (n - 1 - c);
        case 5: return (n - 1 - r) * n + c;
        case 6: return c * n + r;
        default: return (n - 1 - c) * n + (n - 1 - r);
    }
}
/* image() maps (r,c) -> (rr,cc) with the SAME table, so a placement is fixed by t iff
 * board[r][c] == board at symImage(t,r,c) for every cell. For a forced cell we need the
 * cell that maps TO (r,c); for an involution that is the same cell, and for the rotations
 * (t = 1, 3) the preimage is the image under the inverse rotation (3, 1). */
static int symPre(int t, int r, int c) { int inv = (t == 1) ? 3 : (t == 3) ? 1 : t; return symImage(inv, r, c); }

/* split support: at depth split_depth (cells decided), only continue branch #split_index mod split_count */
static int split_depth = -1, split_index = 0, split_count = 1;
static long long split_counter = 0;

static void search(int pos);

/* lines whose LAST cell is `pos`: count those left empty; returns -1 if over budget */
static int closeLines(int pos, int *used) {
    int r = pos / n, c = pos % n, u = 0;
    if (c == n - 1 && lcRow[r] == 0) u++;
    if (r == n - 1 && lcCol[c] == 0) u++;
    if ((r == n - 1 || c == n - 1) && lcD1[r - c + n - 1] == 0) u++;
    if ((r == n - 1 || c == 0) && lcD2[r + c] == 0) u++;
    *used = u;
    return u <= budget;
}

/* last cell (row-major index) of each diagonal line, by line index */
static int lastOfD1[2 * MAXN], lastOfD2[2 * MAXN];

/* Family-coverage bound: every queen lies on exactly one row, one column, one diagonal and
 * one anti-diagonal.  Lines of a family that are still EMPTY but still have cells to come
 * can only be filled by the R queens still to place -- one line per queen per family -- so
 * at least (alive empty lines of the family - R) of them will end empty.  Summed over the
 * four families that must fit in what is left of the empty-line budget. */
static int coverageOk(int pos) {
    int R = NQ - nq, short_ = 0;
    if (pos >= n * n) return 1;
    int rowsLeft = n - pos / n;                   /* rows pos/n .. n-1 are all still empty */
    if (rowsLeft > R) short_ += rowsLeft - R;
    int ec = 0; for (int c = 0; c < n; c++) if (lcCol[c] == 0) ec++;
    if (ec > R) short_ += ec - R;
    int e1 = 0, e2 = 0;
    for (int d = 0; d < 2 * n - 1; d++) {
        if (lcD1[d] == 0 && lastOfD1[d] >= pos) e1++;
        if (lcD2[d] == 0 && lastOfD2[d] >= pos) e2++;
    }
    if (e1 > R) short_ += e1 - R;
    if (e2 > R) short_ += e2 - R;
    return short_ <= budget;
}

static void rowEnd(int pos) {
    /* pos = first cell of the next row (or n*n). Every queen must still be able to reach K. */
    for (int i = 0; i < nq; i++) if (cnt[i] < K && !feasible(i, pos)) { kills[0]++; return; }
    if (!coverageOk(pos)) { kills[1]++; return; }
    search(pos);
}

static void next(int pos) {
    if ((pos % n) == 0) rowEnd(pos); else search(pos);
}

static void search(int pos) {
    nodes++; rowNodes[pos / n]++;
    if (split_depth == pos) { if ((split_counter++ % split_count) != split_index) return; }
    if (pos == n * n) {
        if (nq != NQ) { kills[4]++; return; }
        for (int i = 0; i < nq; i++) if (cnt[i] != K) { kills[5]++; return; }
        if (symg) {
            for (int rr = 0; rr < n; rr++) for (int cc = 0; cc < n; cc++) {
                int im = symImage(symg, rr, cc);
                if (board[rr][cc] != board[im / n][im % n]) return;
            }
        }
        record();
        return;
    }
    int r = pos / n, c = pos % n;
    /* symmetric mode: if this cell's preimage under g is already decided, the choice is forced */
    int forced = -1;
    if (symg) {
        int pre = symPre(symg, r, c);
        if (pre < pos) forced = board[pre / n][pre % n];
    }
    /* A new row: its horizontal line starts empty. The previous row's marker MUST come back
     * on return, or the caller -- still deciding that row's LAST cell -- places a queen
     * without seeing the row's earlier queens. (The first version reset it and never
     * restored it: n=5 found 6 of the 212 placements the brute force finds.) */
    int savedRow = openRow;
    if (c == 0) openRow = -1;
    int cellsLeft = n * n - pos;
    /* --- option 1: no queen here --- */
    if (forced != 1 && NQ - nq <= cellsLeft - 1) {
        int u;
        if (closeLines(pos, &u)) { budget -= u; next(pos + 1); budget += u; } else kills[2]++;
    }
    /* --- option 2: a queen here --- */
    if (forced != 0 && nq < NQ) {
        int q = nq;
        int sRow = openRow, sCol = openCol[c], sD1 = openD1[r - c + n - 1], sD2 = openD2[r + c];
        int ok = 1;
        qr[q] = r; qc[q] = c; cnt[q] = 0;
        int ps[4] = { sRow, sCol, sD1, sD2 };
        for (int k = 0; k < 4; k++) if (ps[k] >= 0) { cnt[ps[k]]++; cnt[q]++; if (cnt[ps[k]] > K) ok = 0; }
        if (cnt[q] > K) ok = 0;
        if (!ok) kills[3]++;
        if (ok) {
            openRow = q; openCol[c] = q; openD1[r - c + n - 1] = q; openD2[r + c] = q;
            lcRow[r]++; lcCol[c]++; lcD1[r - c + n - 1]++; lcD2[r + c]++;
            board[r][c] = 1; nq++;
            int u;
            if (closeLines(pos, &u)) { budget -= u; next(pos + 1); budget += u; }
            nq--; board[r][c] = 0;
            lcRow[r]--; lcCol[c]--; lcD1[r - c + n - 1]--; lcD2[r + c]--;
        }
        for (int k = 0; k < 4; k++) if (ps[k] >= 0) { cnt[ps[k]]--; }
        openRow = sRow; openCol[c] = sCol; openD1[r - c + n - 1] = sD1; openD2[r + c] = sD2;
    }
    openRow = savedRow;
}

int main(int argc, char **argv) {
    if (argc < 4) { fprintf(stderr, "usage: q3 n K N [split_depth split_index split_count]\n"); return 2; }
    n = atoi(argv[1]); K = atoi(argv[2]); NQ = atoi(argv[3]);
    if (argc >= 7) { split_depth = atoi(argv[4]); split_index = atoi(argv[5]); split_count = atoi(argv[6]); }
    { const char *g = getenv("Q3_SYM"); if (g) symg = atoi(g); }
    if (n < 1 || n > 11) { fprintf(stderr, "n must be 1..11 (128-bit board)\n"); return 2; }
    if ((K * NQ) % 2) { printf("n=%d K=%d N=%d: K*N odd, no solutions\n", n, K, NQ); return 0; }
    int nonEmpty = 4 * NQ - K * NQ / 2;
    budget = (6 * n - 2) - nonEmpty;
    if (budget < 0) { printf("n=%d K=%d N=%d: needs %d non-empty lines of %d, impossible\n", n, K, NQ, nonEmpty, 6 * n - 2); return 0; }
    for (int i = 0; i < MAXN; i++) openCol[i] = -1;
    for (int i = 0; i < 2 * MAXN; i++) { openD1[i] = -1; openD2[i] = -1; }
    openRow = -1;
    for (int r = 0; r < n; r++) for (int c = 0; c < n; c++) {
        lastOfD1[r - c + n - 1] = lastD1(r, c);
        lastOfD2[r + c] = lastD2(r, c);
    }
    search(0);
    for (int r = 0; r <= n; r++) fprintf(stderr, "row %d: %lld nodes\n", r, rowNodes[r]);
    fprintf(stderr, "kills: feasible=%lld coverage=%lld budget=%lld overK=%lld endN=%lld endK=%lld\n", kills[0], kills[1], kills[2], kills[3], kills[4], kills[5]);
    printf("n=%d K=%d N=%d budget=%d: total=%lld classes=%lld nodes=%lld\n", n, K, NQ, (6 * n - 2) - nonEmpty, total, classes, nodes);
    return 0;
}
