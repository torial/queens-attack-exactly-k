"""Write each k=2 maximum as its cycle: a cyclic word of moves, one per attacking pair.
Move letters: R L D U (row/column) and the diagonals a=(1,1) b=(-1,-1) c=(1,-1) d=(-1,1),
each followed by its length.  The word starts at the queen nearest the top-left corner and
goes the way that makes the word lexicographically smaller.

usage: python walk.py k2/boards/n9.txt
"""
import sys
from cycles import boards, DIRS

NAME = {(0, 1): 'R', (0, -1): 'L', (1, 0): 'D', (-1, 0): 'U',
        (1, 1): 'a', (-1, -1): 'b', (1, -1): 'c', (-1, 1): 'd'}


def neighbours(qs, n, q):
    r, c = q
    out = []
    for dr, dc in DIRS:
        rr, cc, k = r + dr, c + dc, 1
        while 0 <= rr < n and 0 <= cc < n:
            if (rr, cc) in qs:
                out.append(((rr, cc), (dr, dc), k))
                break
            rr += dr
            cc += dc
            k += 1
    return out


def walk(b):
    n = len(b)
    qs = {(r, c) for r in range(n) for c in range(n) if b[r][c] == 'Q'}
    start = min(qs, key=lambda q: (q[0] + q[1], q[0]))
    words = []
    for first in neighbours(qs, n, start):
        prev, cur, w = start, first[0], [NAME[first[1]] + str(first[2])]
        while cur != start:
            nxt = [x for x in neighbours(qs, n, cur) if x[0] != prev]
            if len(nxt) != 1:
                return None
            prev, cur = cur, nxt[0][0]
            w.append(NAME[nxt[0][1]] + str(nxt[0][2]))
        words.append(w)
    return min(words, key=lambda w: ' '.join(w))


if __name__ == '__main__':
    for path in sys.argv[1:]:
        print('==', path)
        for b in boards(path):
            w = walk(b)
            print(' '.join(w) if w else 'not a single cycle')
