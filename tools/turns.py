"""Group k=2 class representatives by the number of TURNING queens (a queen whose two
attack-neighbours are not on one line) and print their cycle words.

usage: python turns.py k2/boards/n11.txt
"""
import sys
from cycles import boards, anatomy
from walk import walk

for path in sys.argv[1:]:
    groups = {}
    for b in boards(path):
        cyc, straight = anatomy(b)
        t = sum(cyc) - straight
        groups.setdefault(t, []).append((b, walk(b)))
    for t in sorted(groups):
        print(f'== {path}: {len(groups[t])} classes with {t} turning queens')
        for b, w in groups[t]:
            print('  ', ' '.join(w))
