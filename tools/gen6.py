"""Generate the conjectured 6-TURN family of k=2 maxima for odd n, from the rule read off
the n = 11 search output:

  left column rows P = {0, 1, n-1} + one row from each pair (r, n-2-r), r = 2 .. (n-5)/2 + 1
  g = gaps of sorted P   (sum n-1, (n+1)/2 entries... minus one: (n-1)/2 moves)
  word = D(g)  d1  R(g[1:])  b1  U(g[1:] with last-1)  L(g with last-1), from (0,0)

Writes the boards in the search's print format so checkboards.py can judge them, and
reports how many are distinct up to the 8 symmetries.

usage: python gen6.py n > out.txt
"""
import itertools
import sys

STEP = {'D': (1, 0), 'R': (0, 1), 'U': (-1, 0), 'L': (0, -1), 'd': (-1, 1), 'b': (-1, -1)}


def board_from_word(n, word):
    r = c = 0
    qs = [(0, 0)]
    for letter, k in word:
        dr, dc = STEP[letter]
        r, c = r + dr * k, c + dc * k
        if not (0 <= r < n and 0 <= c < n):
            return None
        qs.append((r, c))
    if qs[-1] != (0, 0):
        return None
    return qs[:-1]


def canon(n, qs):
    imgs = []
    for t in range(8):
        img = []
        for r, c in qs:
            rr, cc = [(r, c), (c, n - 1 - r), (n - 1 - r, n - 1 - c), (n - 1 - c, r),
                      (r, n - 1 - c), (n - 1 - r, c), (c, r), (n - 1 - c, n - 1 - r)][t]
            img.append((rr, cc))
        imgs.append(tuple(sorted(img)))
    return min(imgs)


def family(n):
    pairs = [(r, n - 2 - r) for r in range(2, (n - 5) // 2 + 2)]
    for pick in itertools.product(*pairs):
        P = sorted({0, 1, n - 1, *pick})
        g = [b - a for a, b in zip(P, P[1:])]
        word = ([('D', x) for x in g] + [('d', 1)] + [('R', x) for x in g[1:]] + [('b', 1)]
                + [('U', x) for x in g[1:-1] + [g[-1] - 1]]
                + [('L', x) for x in g[:-1] + [g[-1] - 1]])
        yield board_from_word(n, word)


if __name__ == '__main__':
    n = int(sys.argv[1])
    boards = list(family(n))
    broken = sum(b is None for b in boards)
    good = [b for b in boards if b is not None]
    classes = len({canon(n, b) for b in good})
    for i, qs in enumerate(good, 1):
        s = set(qs)
        print(f'class {i}:')
        for r in range(n):
            print(''.join('Q' if (r, c) in s else '.' for c in range(n)))
    sys.stderr.write(f'n={n}: {len(boards)} generated, {broken} did not close, '
                     f'{len(good)} boards, {classes} distinct classes, '
                     f'queens per board {sorted({len(b) for b in good})} (expect {2 * n - 2})\n')
