"""Independent check of printed boards: count each queen's attacks (blocked, 8 directions)
from scratch.  usage: python checkboards.py FILE n K"""
import sys

def attacks(qs, n):
    s = set(qs)
    out = []
    for (r, c) in qs:
        k = 0
        for dr, dc in ((0,1),(0,-1),(1,0),(-1,0),(1,1),(1,-1),(-1,1),(-1,-1)):
            rr, cc = r + dr, c + dc
            while 0 <= rr < n and 0 <= cc < n:
                if (rr, cc) in s:
                    k += 1
                    break
                rr += dr; cc += dc
        out.append(k)
    return out

path, n, K = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
blocks = open(path).read().split('class ')[1:]
bad = 0
for b in blocks:
    rows = [l for l in b.splitlines()[1:] if l and set(l) <= set('Q.')][:n]
    qs = [(r, c) for r, l in enumerate(rows) for c, ch in enumerate(l) if ch == 'Q']
    a = attacks(qs, n)
    ok = all(x == K for x in a)
    bad += not ok
    print(f"{len(qs)} queens, attack counts {sorted(set(a))} -> {'OK' if ok else 'BAD'}")
print(f"{len(blocks)} boards, {bad} bad")
sys.exit(1 if bad else 0)
