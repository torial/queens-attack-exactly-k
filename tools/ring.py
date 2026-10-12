import sys, re
for path in sys.argv[1:]:
    rows=[]; boards=[]
    for line in open(path):
        line=line.strip()
        if line.startswith('class'):
            if rows: boards.append(rows)
            rows=[]
        elif re.fullmatch(r'[Q.]+', line): rows.append(line)
    if rows: boards.append(rows)
    n=len(boards[0]) if boards else 0
    hist={}
    worst=0
    for b in boards:
        d=max(min(r,c,n-1-r,n-1-c) for r,row in enumerate(b) for c,ch in enumerate(row) if ch=='Q')
        hist[d]=hist.get(d,0)+1; worst=max(worst,d)
    print(path, 'n=',n,'classes=',len(boards),'max ring depth histogram',hist)
