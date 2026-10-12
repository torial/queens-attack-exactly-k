"""Generate the conjectured ODD-n 7-turn family, read off n = 11 and checked against 9, 13.
Every 7-turn class is a 6-turn base word (gaps g, see gen6.py) with ONE kink moved:

  Lead = number of leading 1s in g;  j = 1 .. Lead-1
  b-type: D(g) d1 R(g[1:]) U(u[:j]) b1 U(u[j:]) L(g[:-1] + [g[-1]-1]),  u = g[1:-1] + [g[-1]-1]
  a-type: D(g[:j]) a1 D(g[j+1:]) d1 R(u) U([1] + u) L(g),             u = g[1:-1] + [g[-1]-1]

usage: python gen7odd.py n > out.txt     (summary on stderr)
"""
import itertools
import sys
from gen6 import canon
from gen_even import board_from_word


def bases(n):
    pairs = [(r, n - 2 - r) for r in range(2, (n - 5) // 2 + 2)]
    for pick in itertools.product(*pairs):
        P = sorted({0, 1, n - 1, *pick})
        yield [b - a for a, b in zip(P, P[1:])]


def lead_ones(g):
    k = 0
    while k < len(g) and g[k] == 1:
        k += 1
    return k


def family(n):
    for g in bases(n):
        u = g[1:-1] + [g[-1] - 1]
        for j in range(1, lead_ones(g)):
            S = lambda letter, xs: [(letter, x) for x in xs]
            b_word = (S('D', g) + [('d', 1)] + S('R', g[1:]) + S('U', u[:j]) + [('b', 1)]
                      + S('U', u[j:]) + S('L', g[:-1] + [g[-1] - 1]))
            a_word = (S('D', g[:j]) + [('a', 1)] + S('D', g[j + 1:]) + [('d', 1)] + S('R', u)
                      + S('U', [1] + u) + S('L', g))
            yield 'b', board_from_word(n, b_word, (0, 0))
            yield 'a', board_from_word(n, a_word, (0, 0))


if __name__ == '__main__':
    n = int(sys.argv[1])
    made = list(family(n))
    good = [q for _, q in made if q is not None]
    for i, qs in enumerate(good, 1):
        s = set(qs)
        print(f'class {i}:')
        for r in range(n):
            print(''.join('Q' if (r, c) in s else '.' for c in range(n)))
    sys.stderr.write(f'n={n}: {len(made)} generated ({sum(t == "a" for t, _ in made)} a, '
                     f'{sum(t == "b" for t, _ in made)} b), {len(made) - len(good)} broken, '
                     f'{len({canon(n, q) for q in good})} distinct classes; '
                     f'predicted 2^((n-3)/2)-2 = {2 ** ((n - 3) // 2) - 2}\n')
