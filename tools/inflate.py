"""Test the conjectured INFLATION n-2 -> n for odd-n k=2 maxima.

Take each class representative at n-2, write its cycle word (walk.py: starts at the queen
nearest the top-left corner, going down), split it into its four sides -- a diagonal kink
belongs to the side of the NEXT straight move -- then in every side prepend a 1-step of that
side's direction and lengthen its last move by 1.  Build the n board from (0,0).

usage: python inflate.py n_small > out.txt   (reads data/boards/k2_n<n_small>.txt)     (summary on stderr)
"""
import os
import re
import sys
from cycles import boards
from walk import walk
from gen6 import canon
from gen_even import board_from_word

STRAIGHT = 'DRUL'


def inflate(word):
    moves = [(m[0], int(m[1:])) for m in word]
    sides, pending = [], []
    cur = None
    for letter, k in moves:
        if letter not in STRAIGHT:
            pending.append((letter, k))
            continue
        if letter != cur:
            sides.append([letter, []])
            cur = letter
        sides[-1][1].extend(pending)
        pending = []
        sides[-1][1].append((letter, k))
    if pending or [s[0] for s in sides] != list(STRAIGHT):
        return None
    out = []
    for letter, ms in sides:
        ms = [(letter, 1)] + ms
        last_letter, last_k = ms[-1]
        if last_letter != letter:
            return None
        ms[-1] = (letter, last_k + 1)
        out += ms
    return out


if __name__ == '__main__':
    m = int(sys.argv[1])
    n = m + 2
    made, broken = [], 0
    src = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'data', 'boards', f'k2_n{m}.txt')
    for b in boards(src):
        w = walk(b)
        nw = inflate(w) if w else None
        qs = board_from_word(n, nw, (0, 0)) if nw else None
        if qs is None:
            broken += 1
            continue
        made.append(qs)
    for i, qs in enumerate(made, 1):
        s = set(qs)
        print(f'class {i}:')
        for r in range(n):
            print(''.join('Q' if (r, c) in s else '.' for c in range(n)))
    sys.stderr.write(f'inflate {m} -> {n}: {len(made)} built, {broken} could not be inflated, '
                     f'{len({canon(n, q) for q in made})} distinct classes\n')
