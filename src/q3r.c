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

#define MAXN 40
static int ring = 1000;   /* Q3_RING: queens only on cells within `ring` of the edge */
static int printReps = 0; /* Q3_PRINT: print each class representative */
static int depthOf(int r, int c);
static int n, K, NQ;
static int budget;                    /* empty lines still allowed */
static int qr[MAXN * MAXN], qc[MAXN * MAXN], cnt[MAXN * MAXN];
static int nq;
static int openRow, openCol[MAXN], openD1[2 * MAXN], openD2[2 * MAXN];
static int lcRow[MAXN], lcCol[MAXN], lcD1[2 * MAXN], lcD2[2 * MAXN];  /* queens on line */
static unsigned char board[MAXN][MAXN];
static long long total, classes;
static long long nodes;

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
/* Canonical form: compare the 8 images of the board as n*n-cell arrays, the HIGHEST cell
 * index most significant -- the same order the earlier 128-bit masks compared in, so the
 * same representative is chosen, but with no limit on the board size. */
static unsigned char imgbuf[8][MAXN * MAXN];
static void imageInto(int t, unsigned char *m) {
    memset(m, 0, (size_t)n * n);
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
        m[rr * n + cc] = 1;
    }
}
/* <0 if a < b in that order */
static int cmpImg(const unsigned char *a, const unsigned char *b) {
    for (int i = n * n - 1; i >= 0; i--) if (a[i] != b[i]) return a[i] < b[i] ? -1 : 1;
    return 0;
}
static void record(void) {
    imageInto(0, imgbuf[0]);
    int smaller = 0;
    for (int t = 1; t < 8 && !smaller; t++) { imageInto(t, imgbuf[t]); if (cmpImg(imgbuf[t], imgbuf[0]) < 0) smaller = 1; }
    total++;
    if (!smaller) {
        classes++;
        if (printReps) {
            printf("class %lld:\n", classes);
            for (int r = 0; r < n; r++) { for (int c = 0; c < n; c++) putchar(board[r][c] ? 'Q' : '.'); putchar('\n'); }
        }
    }
}
static int depthOf(int r, int c) { int d = r; if (c < d) d = c; if (n - 1 - r < d) d = n - 1 - r; if (n - 1 - c < d) d = n - 1 - c; return d; }

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
/* Propagation at a row boundary (pos = first cell of a row).  A future cell is FORBIDDEN if
 * one of its lines ends at a queen that already attacks K others -- a queen there would make
 * it K+1.  (Future rows hold no queens yet, so the row line never forbids.)  Then:
 *   - an EMPTY line with no allowed cell left will end empty: it spends budget NOW;
 *   - a needy queen's line counts toward its potential only if an allowed cell remains on it.
 * Both conditions were previously discovered only when the line's last cell was passed --
 * near the bottom of the board, which is where the profile showed the search living. */
static int propagateOk(int pos) {
    if (pos >= n * n) return 1;
    int r0 = pos / n;
    static unsigned char allowed[MAXN][MAXN];
    for (int r = r0; r < n; r++) for (int c = 0; c < n; c++) {
        int a = 1;
        int q;
        if ((q = openCol[c]) >= 0 && cnt[q] >= K) a = 0;
        if (a && (q = openD1[r - c + n - 1]) >= 0 && cnt[q] >= K) a = 0;
        if (a && (q = openD2[r + c]) >= 0 && cnt[q] >= K) a = 0;
        if (depthOf(r, c) > ring) a = 0;
        allowed[r][c] = (unsigned char)a;
    }
    /* per line: does an allowed future cell remain? */
    static unsigned char colOk[MAXN], d1Ok[2 * MAXN], d2Ok[2 * MAXN];
    memset(colOk, 0, sizeof colOk); memset(d1Ok, 0, sizeof d1Ok); memset(d2Ok, 0, sizeof d2Ok);
    int deadEmpty = 0;
    for (int r = r0; r < n; r++) {
        int rowAny = 0;
        for (int c = 0; c < n; c++) if (allowed[r][c]) {
            rowAny = 1; colOk[c] = 1; d1Ok[r - c + n - 1] = 1; d2Ok[r + c] = 1;
        }
        if (!rowAny) deadEmpty++;                 /* this whole future row stays empty */
    }
    for (int c = 0; c < n; c++) if (lcCol[c] == 0 && !colOk[c]) deadEmpty++;
    for (int d = 0; d < 2 * n - 1; d++) {
        if (lcD1[d] == 0 && lastOfD1[d] >= pos && !d1Ok[d]) deadEmpty++;
        if (lcD2[d] == 0 && lastOfD2[d] >= pos && !d2Ok[d]) deadEmpty++;
    }
    if (deadEmpty > budget) return 0;
    /* needy queens: potential only through lines that can still receive a queen */
    for (int i = 0; i < nq; i++) if (cnt[i] < K) {
        int r = qr[i], c = qc[i], pot = 0;
        if (openCol[c] == i && colOk[c]) pot++;
        if (openD1[r - c + n - 1] == i && d1Ok[r - c + n - 1]) pot++;
        if (openD2[r + c] == i && d2Ok[r + c]) pot++;
        if (cnt[i] + pot < K) return 0;
    }
    /* enough allowed cells for the queens still to place */
    int cells = 0;
    for (int r = r0; r < n; r++) for (int c = 0; c < n; c++) cells += allowed[r][c];
    if (cells < NQ - nq) return 0;
    return 1;
}

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
    for (int i = 0; i < nq; i++) if (cnt[i] < K && !feasible(i, pos)) return;
    if (!coverageOk(pos)) return;
    if (!propagateOk(pos)) return;
    search(pos);
}

static void next(int pos) {
    if ((pos % n) == 0) rowEnd(pos); else search(pos);
}

static void search(int pos) {
    nodes++;
    if (split_depth == pos) { if ((split_counter++ % split_count) != split_index) return; }
    if (pos == n * n) {
        if (nq != NQ) return;
        for (int i = 0; i < nq; i++) if (cnt[i] != K) return;
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
        if (closeLines(pos, &u)) { budget -= u; next(pos + 1); budget += u; }
    }
    /* --- option 2: a queen here --- */
    if (forced != 0 && nq < NQ && depthOf(r, c) <= ring) {
        int q = nq;
        int sRow = openRow, sCol = openCol[c], sD1 = openD1[r - c + n - 1], sD2 = openD2[r + c];
        int ok = 1;
        qr[q] = r; qc[q] = c; cnt[q] = 0;
        int ps[4] = { sRow, sCol, sD1, sD2 };
        for (int k = 0; k < 4; k++) if (ps[k] >= 0) { cnt[ps[k]]++; cnt[q]++; if (cnt[ps[k]] > K) ok = 0; }
        if (cnt[q] > K) ok = 0;
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
    { const char *g = getenv("Q3_RING"); if (g) ring = atoi(g); }
    { const char *g = getenv("Q3_PRINT"); if (g) printReps = atoi(g); }
    if (n < 1 || n > MAXN - 1) { fprintf(stderr, "n must be 1..%d\n", MAXN - 1); return 2; }
    if (ring < 1000) printf("RING-RESTRICTED: queens only within %d of the edge (a LOWER bound unless the restriction is proved)\n", ring);
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
    printf("n=%d K=%d N=%d budget=%d: total=%lld classes=%lld nodes=%lld\n", n, K, NQ, (6 * n - 2) - nonEmpty, total, classes, nodes);
    return 0;
}
