"""Build the conjectured COMPLETE set of odd-n k=2 maxima with no search:
    all(5) = the one 6-turn class;  all(n) = gen6(n) u gen7odd(n) u inflate(all(n-2))
Prints per-n class counts against 3*2^((n-3)/2) - n, and writes boards for the last n.

usage: python genodd.py N_max > boards_Nmax.txt    (table on stderr)
"""
import sys
from gen6 import canon, family as fam6
from gen7odd import family as fam7
from gen_even import board_from_word
from inflate import inflate
from walk import walk


def to_rows(n, qs):
    s = set(qs)
    return [''.join('Q' if (r, c) in s else '.' for c in range(n)) for r in range(n)]


def build(nmax):
    prev, n = None, 5
    while n <= nmax:
        parts = {'6': [q for q in fam6(n) if q], '7': [q for _, q in fam7(n) if q], '8': []}
        if prev is not None:
            for qs in prev:
                w = walk(to_rows(n - 2, qs))
                nw = inflate(w) if w else None
                b = board_from_word(n, nw, (0, 0)) if nw else None
                if b is None:
                    raise SystemExit(f'n={n}: an n-2 class could not be inflated')
                parts['8'].append(b)
        allq = parts['6'] + parts['7'] + parts['8']
        cls = {canon(n, q) for q in allq}
        pred = 3 * 2 ** ((n - 3) // 2) - n
        sys.stderr.write(f'n={n}: 6-turn {len(parts["6"])}, 7-turn {len(parts["7"])}, '
                         f'8-turn {len(parts["8"])}; {len(cls)} distinct of {len(allq)}; '
                         f'formula {pred} {"OK" if len(cls) == pred else "MISMATCH"}\n')
        # keep one representative per class for the next inflation
        seen, keep = set(), []
        for q in allq:
            k = canon(n, q)
            if k not in seen:
                seen.add(k)
                keep.append(q)
        prev, last_n = keep, n
        n += 2
    return last_n, prev


if __name__ == '__main__':
    n, qs_list = build(int(sys.argv[1]))
    for i, qs in enumerate(qs_list, 1):
        print(f'class {i}:')
        print('\n'.join(to_rows(n, qs)))
