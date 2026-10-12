/* tm.c -- transfer-matrix count of placements of N queens on an n x n board in which every
 * queen attacks exactly K others (attacks along rows, columns and diagonals, blocked by the
 * nearest queen).  Counts ALL placements (not up to symmetry); classes come from Burnside
 * with q3's symmetric-mode counts.  An independent method for the same number q3's search
 * produces, so the two can check each other.
 *
 * Cells are decided in row-major order.  Between two cells the future can see only:
 *   - for each line (row, column, diagonal, anti-diagonal) still being filled:
 *       NEVER   no queen on it yet
 *       FORBID  it has queens and its last one already attacks K others: nothing more may go
 *               on this line (one more queen there would make K+1)
 *       label   its last queen still needs attacks; the label identifies that queen, because
 *               one queen can be the last on several lines at once
 *   - each needy queen's current attack count
 *   - queens placed so far, and empty lines used so far (at most E may stay empty; see q3.c)
 * Finished lines are all marked DONE, so they never distinguish two states.
 *
 * usage: tm n K N
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAXL 70          /* 6n-2 lines; n <= 12 */
#define MAXLAB 40
enum { NEVER = 0, FORBID = 1, DONE = 2, LAB0 = 3 };

static int n, K, NQ, L, E, KEYLEN;

/* ---- state hash map (open addressing), keys are KEYLEN bytes, values uint64 ---- */
typedef struct { unsigned char *keys; uint64_t *vals; size_t cap, size; } Map;
static uint64_t hashKey(const unsigned char *k) {
    uint64_t h = 1469598103934665603ULL;
    for (int i = 0; i < KEYLEN; i++) { h ^= k[i]; h *= 1099511628211ULL; }
    return h ^ (h >> 29);
}
static void mapInit(Map *m, size_t cap) {
    m->cap = cap; m->size = 0;
    m->keys = calloc(cap, KEYLEN); m->vals = calloc(cap, sizeof(uint64_t));
    if (!m->keys || !m->vals) { fprintf(stderr, "out of memory (cap %zu)\n", cap); exit(3); }
}
static void mapFree(Map *m) { free(m->keys); free(m->vals); m->keys = NULL; m->vals = NULL; }
static void mapAdd(Map *m, const unsigned char *k, uint64_t v);
static void mapGrow(Map *m) {
    Map nm; mapInit(&nm, m->cap * 2);
    for (size_t i = 0; i < m->cap; i++) if (m->vals[i]) mapAdd(&nm, m->keys + i * KEYLEN, m->vals[i]);
    mapFree(m); *m = nm;
}
static void mapAdd(Map *m, const unsigned char *k, uint64_t v) {
    if ((m->size + 1) * 10 > m->cap * 7) mapGrow(m);
    size_t i = hashKey(k) & (m->cap - 1);
    for (;;) {
        if (!m->vals[i]) { memcpy(m->keys + i * KEYLEN, k, KEYLEN); m->vals[i] = v; m->size++; return; }
        if (!memcmp(m->keys + i * KEYLEN, k, KEYLEN)) { m->vals[i] += v; return; }
        i = (i + 1) & (m->cap - 1);
    }
}

/* ---- working (decoded) state ---- */
static unsigned char code[MAXL];   /* NEVER / FORBID / DONE / LAB0+label */
static int lcount[MAXLAB + 8];     /* attack count of needy queen `label` (+ room for the temporary new-queen label) */
static int nlab, nq, used;

static void decode(const unsigned char *k) {
    memcpy(code, k, L);
    nlab = 0;
    for (int i = 0; i < MAXLAB + 8; i++) lcount[i] = 0;
    for (int i = 0; i < L; i++) if (code[i] >= LAB0 && code[i] - LAB0 + 1 > nlab) nlab = code[i] - LAB0 + 1;
    for (int i = 0; i < nlab; i++) lcount[i] = k[L + i];
    nq = k[KEYLEN - 2]; used = k[KEYLEN - 1];
}
/* canonical encode: relabel needy queens by first appearance in line order */
static int encode(unsigned char *k) {
    int map[MAXLAB + 8]; for (int i = 0; i < MAXLAB + 8; i++) map[i] = -1;
    int next = 0;
    memset(k, 0, KEYLEN);
    for (int i = 0; i < L; i++) {
        if (code[i] >= LAB0) {
            int l = code[i] - LAB0;
            if (map[l] < 0) { if (next >= MAXLAB) return 0; map[l] = next++; }
            k[i] = LAB0 + map[l];
        } else k[i] = code[i];
    }
    for (int l = 0; l < MAXLAB + 8; l++) if (map[l] >= 0) k[L + map[l]] = lcount[l];
    k[KEYLEN - 2] = nq; k[KEYLEN - 1] = used;
    return 1;
}

static int lineRow(int r) { return r; }
static int lineCol(int c) { return n + c; }
static int lineD1(int r, int c) { return 2 * n + (r - c + n - 1); }
static int lineD2(int r, int c) { return 2 * n + (2 * n - 1) + (r + c); }

/* every needy queen must still head at least K - count lines */
static int feasibleAll(void) {
    int heads[MAXLAB + 8] = {0};
    for (int i = 0; i < L; i++) if (code[i] >= LAB0) heads[code[i] - LAB0]++;
    for (int l = 0; l < MAXLAB + 8; l++) {
        if (heads[l] == 0) {
            /* a queen that heads nothing is gone; if it was needy it is dead -- but a label
             * with no lines does not exist in the state, so nothing to check here except
             * labels that were just stripped of their last line (handled by callers) */
            continue;
        }
        if (heads[l] < K - lcount[l]) return 0;
    }
    return 1;
}

int main(int argc, char **argv) {
    if (argc < 4) { fprintf(stderr, "usage: tm n K N\n"); return 2; }
    n = atoi(argv[1]); K = atoi(argv[2]); NQ = atoi(argv[3]);
    L = 6 * n - 2;
    if (L > MAXL) { fprintf(stderr, "n too large\n"); return 2; }
    if ((K * NQ) % 2) { printf("n=%d K=%d N=%d total=0 (K*N odd)\n", n, K, NQ); return 0; }
    E = L - (4 * NQ - K * NQ / 2);
    if (E < 0) { printf("n=%d K=%d N=%d total=0 (too many lines)\n", n, K, NQ); return 0; }
    KEYLEN = L + MAXLAB + 2;

    Map cur, nxt;
    mapInit(&cur, 1 << 10);
    unsigned char key[256], key2[256];
    memset(code, NEVER, sizeof code); nlab = 0; nq = 0; used = 0;
    for (int i = 0; i < MAXLAB; i++) lcount[i] = 0;
    encode(key); mapAdd(&cur, key, 1);
    size_t peak = 0;

    for (int pos = 0; pos < n * n; pos++) {
        int r = pos / n, c = pos % n;
        int ln[4] = { lineRow(r), lineCol(c), lineD1(r, c), lineD2(r, c) };
        int ends[4] = { c == n - 1, r == n - 1, (r == n - 1 || c == n - 1), (r == n - 1 || c == 0) };
        int cellsAfter = n * n - pos - 1;
        mapInit(&nxt, 1 << 10);
        for (size_t si = 0; si < cur.cap; si++) {
            uint64_t v = cur.vals[si];
            if (!v) continue;
            const unsigned char *sk = cur.keys + si * KEYLEN;
            for (int choice = 0; choice < 2; choice++) {
                decode(sk);
                int ok = 1;
                if (choice == 1) {
                    if (nq >= NQ) continue;
                    int x = MAXLAB + 7;      /* temporary label for the new queen */
                    int a = 0;
                    int touched[4], nt = 0;
                    for (int k = 0; k < 4 && ok; k++) {
                        int cd = code[ln[k]];
                        if (cd == FORBID) ok = 0;
                        else if (cd >= LAB0) { int q = cd - LAB0; lcount[q]++; a++; touched[nt++] = q; if (lcount[q] > K) ok = 0; }
                    }
                    if (!ok || a > K) continue;
                    for (int k = 0; k < 4; k++) code[ln[k]] = LAB0 + x;
                    lcount[x] = a;
                    nq++;
                    /* an old queen that reached K: its other lines are now forbidden */
                    for (int t = 0; t < nt; t++) {
                        int q = touched[t];
                        if (lcount[q] == K) for (int i = 0; i < L; i++) if (code[i] == LAB0 + q) code[i] = FORBID;
                    }
                    if (a == K) for (int k = 0; k < 4; k++) code[ln[k]] = FORBID;
                    /* an old needy queen that lost a line and now heads none is dead */
                    for (int t = 0; t < nt && ok; t++) {
                        int q = touched[t];
                        if (lcount[q] < K) {
                            int h = 0; for (int i = 0; i < L; i++) if (code[i] == LAB0 + q) h++;
                            if (h == 0) ok = 0;
                        }
                    }
                    if (!ok) continue;
                }
                /* lines whose last cell is this one */
                for (int k = 0; k < 4 && ok; k++) if (ends[k]) {
                    int cd = code[ln[k]];
                    if (cd == NEVER) { used++; if (used > E) ok = 0; }
                    else if (cd >= LAB0) {
                        int q = cd - LAB0;
                        code[ln[k]] = DONE;
                        int h = 0; for (int i = 0; i < L; i++) if (code[i] == LAB0 + q) h++;
                        if (h < K - lcount[q]) ok = 0;   /* includes h == 0 with count < K */
                    }
                    code[ln[k]] = DONE;
                }
                if (!ok) continue;
                if (!feasibleAll()) continue;
                if (NQ - nq > cellsAfter) continue;
                if (!encode(key2)) { fprintf(stderr, "label overflow at pos %d\n", pos); return 4; }
                mapAdd(&nxt, key2, v);
            }
        }
        mapFree(&cur); cur = nxt;
        if (cur.size > peak) peak = cur.size;
        if (getenv("TM_VERBOSE")) fprintf(stderr, "pos %d: %zu states\n", pos, cur.size);
    }
    uint64_t total = 0;
    for (size_t si = 0; si < cur.cap; si++) if (cur.vals[si]) {
        decode(cur.keys + si * KEYLEN);
        if (nq == NQ) total += cur.vals[si];
    }
    printf("n=%d K=%d N=%d E=%d: total=%llu peak_states=%zu\n", n, K, NQ, E, (unsigned long long)total, peak);
    return 0;
}
