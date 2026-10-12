"""Brute force for small n: which reading of "attacks" reproduces A051759?  Counts placements
of N queens where every queen attacks exactly K others, up to the 8 square symmetries, under
(a) blocked attacks (nearest queen in each of 8 directions) and (b) unblocked (every queen on
a shared line)."""
import itertools, sys

def attacks(qs, n, blocked):
    s = set(qs)
    out = []
    for (r, c) in qs:
        k = 0
        for dr, dc in ((0,1),(0,-1),(1,0),(-1,0),(1,1),(1,-1),(-1,1),(-1,-1)):
            rr, cc = r + dr, c + dc
            while 0 <= rr < n and 0 <= cc < n:
                if (rr, cc) in s:
                    k += 1
                    if blocked:
                        break
                rr += dr; cc += dc
        out.append(k)
    return out

def images(qs, n):
    fs = [lambda r,c:(r,c), lambda r,c:(c,n-1-r), lambda r,c:(n-1-r,n-1-c), lambda r,c:(n-1-c,r),
          lambda r,c:(r,n-1-c), lambda r,c:(n-1-r,c), lambda r,c:(c,r), lambda r,c:(n-1-c,n-1-r)]
    return [tuple(sorted(f(r,c) for r,c in qs)) for f in fs]

def count(n, K, N, blocked):
    cells = [(r,c) for r in range(n) for c in range(n)]
    total = 0; canon = set()
    for qs in itertools.combinations(cells, N):
        if all(a == K for a in attacks(qs, n, blocked)):
            total += 1
            canon.add(min(images(qs, n)))
    return total, len(canon)

for n, N, exp in ((2,4,1),(3,6,2),(4,8,4),(5,10,31)):
    for blocked in (True, False):
        t, c = count(n, 3, N, blocked)
        print(f"n={n} N={N} blocked={blocked}: total={t} classes={c} (A051759 says {exp})")
