"""Compare two board files as SETS OF CLASSES (canonical under the 8 symmetries).
usage: python cmpall.py n search_file generated_file    exit 0 iff equal and non-empty"""
import sys
from cycles import boards
from gen6 import canon

n, a, b = int(sys.argv[1]), sys.argv[2], sys.argv[3]


def classes(path):
    return {canon(n, [(r, c) for r, row in enumerate(bd) for c, ch in enumerate(row) if ch == 'Q'])
            for bd in boards(path)}


A, B = classes(a), classes(b)
print(f'n={n}: {a} {len(A)}, {b} {len(B)}, common {len(A & B)}, '
      f'only-first {len(A - B)}, only-second {len(B - A)}')
sys.exit(0 if A == B and A else 1)
