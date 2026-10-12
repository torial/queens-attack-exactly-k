"""Print only the class representatives with exactly t turning queens.
usage: python filt_turns.py FILE t > out.txt"""
import sys
from cycles import boards, anatomy

path, t = sys.argv[1], int(sys.argv[2])
k = 0
for b in boards(path):
    cyc, straight = anatomy(b)
    if cyc is not None and sum(cyc) - straight == t:
        k += 1
        print(f'class {k}:')
        print('\n'.join(b))
sys.stderr.write(f'{path}: {k} boards with {t} turns\n')
