#!/usr/bin/python3

import sys
import os
from unidecode import unidecode

good = 'abcdefghijklmnopqrstuvwxyz'
words = []

infn = sys.argv[1]
outfn = sys.argv[2]
word_length = int(sys.argv[3]) if len(sys.argv) > 3 else 0
with open(infn, 'r') as inf:
    for line in inf.readlines():
        line = line[:-1]
        line = unidecode(line)
        if not any(i not in good for i in line):
            if word_length==0 or len(line)==word_length:
                words.append(line)
with open(outfn, 'w') as outf:
    outf.write('\n'.join(dict.fromkeys(words).keys()))
    outf.write('\n')
             
