#!/usr/bin/python3

import sys
import os

infn = sys.argv[1]
outfn = sys.argv[2]
with open(infn, 'r') as inf:
    with open(outfn, 'w') as outf:
        input = inf.read()
        start = 0
        while True:
            i = input.find(" &gt;", start)
            if i < 0:
                break
            j = input.find("<span", i)
            word = input[i+5:j]
            start = j
            outf.write(word + '\n')
        
