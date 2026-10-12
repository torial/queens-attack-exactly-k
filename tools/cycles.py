"""Attack-graph anatomy of k=2 boards: every queen attacks exactly 2 others, so the attack
graph is 2-regular, i.e. a disjoint union of cycles.  For each class representative print the
number of cycles, and each queen's line usage: how many of its 4 lines it shares.

usage: python cycles.py k2/boards/n9.txt [...]
"""
import re
import sys

DIRS = [(0, 1), (0, -1), (1, 0), (-1, 0), (1, 1), (-1, -1), (1, -1), (-1, 1)]


def boards(path):
    rows, out = [], []
    for line in open(path):
        line = line.strip()
        if line.startswith('class'):
            if rows:
                out.append(rows)
            rows = []
        elif re.fullmatch(r'[Q.]+', line):
            rows.append(line)
    if rows:
        out.append(rows)
    return out


def anatomy(b):
    n = len(b)
    qs = {(r, c) for r in range(n) for c in range(n) if b[r][c] == 'Q'}
    adj = {}
    for (r, c) in qs:
        nb = []
        for dr, dc in DIRS:
            rr, cc = r + dr, c + dc
            while 0 <= rr < n and 0 <= cc < n:
                if (rr, cc) in qs:
                    nb.append(((rr, cc), (dr, dc)))
                    break
                rr += dr
                cc += dc
        adj[(r, c)] = nb
    bad = [q for q in qs if len(adj[q]) != 2]
    if bad:
        return None, f'NOT k=2: {bad}'
    seen, cyc = set(), []
    for q in sorted(qs):
        if q in seen:
            continue
        stack, comp = [q], 0
        while stack:
            x = stack.pop()
            if x in seen:
                continue
            seen.add(x)
            comp += 1
            stack.extend(y for y, _ in adj[x])
        cyc.append(comp)
    # "straight" queens: both neighbours on one line (opposite directions)
    straight = sum(1 for q in qs if adj[q][0][1] == tuple(-d for d in adj[q][1][1]))
    return cyc, straight


if __name__ == '__main__':
    for path in sys.argv[1:]:
        bs = boards(path)
        summary = {}
        for b in bs:
            cyc, straight = anatomy(b)
            if cyc is None:
                print(path, straight)
                continue
            key = (tuple(sorted(cyc)), straight)
            summary[key] = summary.get(key, 0) + 1
        print(path, f'{len(bs)} classes;', '{(cycle sizes), straight-through queens: count}', summary)
