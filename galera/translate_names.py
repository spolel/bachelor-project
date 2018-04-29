#! /usr/bin/env python3

import sys
import os
from subprocess import PIPE, run

for line in sys.stdin:
    if("Procedure" not in line):
        result = run(["c++filt", "-n"], input=bytes(line.split(";")[0], 'utf-8'), stdout=PIPE, stderr=PIPE)
        #print(line.split(";")[0])
        newline = result.stdout.decode("utf-8").strip('\n') + ";" + line.strip('\n')
        print(newline)
