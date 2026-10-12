"""Generate the conjectured EVEN-n families of k=2 maxima, read off n = 8, 10, 12:

7-turn family (2^((n-8)/2) classes):
  P = {0, 1, 2, n-4, n-2} + one row from each pair (r, n-1-r), r = 4 .. (n-2)/2
  g = gaps of sorted P
  word = D(g) a1 R(g[1:-1]) d2 R1 U(g[2:-1] + [g[-1]+1]) L([2] + g[2:-1] + [g[-1]+1])

10-turn sporadic (1 class, n >= 8):
  D2^(n/2-1) R1 R2 d1 R2^((n-8)/2) R3 U2^(n/2-2) L(n-3) U2 R(n-4) c1 L2^((n-8)/2) L5

usage: python gen_even.py n > out.txt     (summary on stderr)
"""
import itertools
import sys
from gen6 import canon

STEP = {'D': (1, 0), 'R': (0, 1), 'U': (-1, 0), 'L': (0, -1),
        'a': (1, 1), 'b': (-1, -1), 'c': (1, -1), 'd': (-1, 1)}


def board_from_word(n, word, start):
    r, c = start
    qs = [start]
    for letter, k in word:
        dr, dc = STEP[letter]
        r, c = r + dr * k, c + dc * k
        if not (0 <= r < n and 0 <= c < n):
            return None
        qs.append((r, c))
    if qs[-1] != start or len(set(qs[:-1])) != len(qs) - 1:
        return None
    return qs[:-1]


def seven(n):
    pairs = [(r, n - 1 - r) for r in range(4, (n - 2) // 2 + 1)]
    for pick in itertools.product(*pairs):
        P = sorted({0, 1, 2, n - 4, n - 2, *pick})
        g = [b - a for a, b in zip(P, P[1:])]
        tail = g[2:-1] + [g[-1] + 1]
        word = ([('D', x) for x in g] + [('a', 1)] + [('R', x) for x in g[1:-1]]
                + [('d', 2), ('R', 1)] + [('U', x) for x in tail]
                + [('L', x) for x in [2] + tail])
        yield board_from_word(n, word, (0, 0))


def ten(n):
    h = n // 2
    word = ([('D', 2)] * (h - 1) + [('R', 1), ('R', 2), ('d', 1)] + [('R', 2)] * ((n - 8) // 2)
            + [('R', 3)] + [('U', 2)] * (h - 2) + [('L', n - 3), ('U', 2), ('R', n - 4), ('c', 1)]
            + [('L', 2)] * ((n - 8) // 2) + [('L', 5)])
    # the search's word starts at the queen nearest the top-left: (1,0) for this family
    yield board_from_word(n, word, (1, 0))


if __name__ == '__main__':
    n = int(sys.argv[1])
    out, summary = [], []
    for name, fam in (('7-turn', seven), ('10-turn', ten)):
        bs = list(fam(n))
        good = [b for b in bs if b is not None]
        summary.append(f'{name}: {len(bs)} generated, {len(bs) - len(good)} broken, '
                       f'{len({canon(n, b) for b in good})} classes')
        out += good
    allc = len({canon(n, b) for b in out})
    for i, qs in enumerate(out, 1):
        s = set(qs)
        print(f'class {i}:')
        for r in range(n):
            print(''.join('Q' if (r, c) in s else '.' for c in range(n)))
    sys.stderr.write(f'n={n}: ' + '; '.join(summary) + f'; {allc} distinct overall, '
                     f'queens {sorted({len(b) for b in out})} (expect {2 * n - 2})\n')
