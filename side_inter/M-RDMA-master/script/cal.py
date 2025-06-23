import sys

out = dict()
out['-a-b-w'] = dict()
out['-a-w'] = dict()
out['-b-w'] = dict()
out['-w'] = dict()
out['-a-b-r'] = dict()
out['-a-r'] = dict()
out['-b-r'] = dict()
out['-r'] = dict()

# out = {'-a -b': dict(), '-a':dict(), '-b':dict(), '':dict()}

for line in sys.stdin:
    segments = line.split('[TPUT]')
    types = segments[0].strip('=').split()
    size = int(types.pop())
    key = reduce(lambda x,y: x + y, types)
    out[key][size] = out[key].get(size, 0) + float(segments[1])

for key_name, res in out.iteritems():
    print key_name
    for key, value in sorted(res.iteritems()):
        print key, value / 4