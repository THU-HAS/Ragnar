import sys

batch_type = sys.argv[1]

# print batch_type

out = dict()
for i in [2 ** i for i in range(10)]:
    out[i] = dict()
# out['a'] = dict()
# out['b'] = dict()
# out['c'] = dict()

# out = {'-a -b': dict(), '-a':dict(), '-b':dict(), '':dict()}

for line in sys.stdin:
    if batch_type not in line or 'TPUT' not in line:
        continue
    segments = line.split('[TPUT]')
    types = segments[0].strip('=').split()
    size = int(types.pop())
    batch_size = int(types.pop())
    key = reduce(lambda x,y: x + y, types)
    out[batch_size][size] = out[batch_size].get(size, 0) + float(segments[1])

for key_name, res in out.iteritems():
    print key_name
    for key, value in sorted(res.iteritems()):
        print key, value / 4