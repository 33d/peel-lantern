# Generates BufferInput::lookup.

brightness = 3
n = 4096.0 / (256.0 ** brightness)
a = [int(n * (i ** brightness)) for i in range(256)]

for i in xrange(0, 256, 8):
    line = a[i:i+8]
    print "    %s" % reduce(lambda s, v: s + "%4d, " % v, line, "")

