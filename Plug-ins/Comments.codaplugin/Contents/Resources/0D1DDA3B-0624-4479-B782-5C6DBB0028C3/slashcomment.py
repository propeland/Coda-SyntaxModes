#!/usr/bin/env python

import os

text = os.environ['CODA_SELECTED_TEXT'].split("\n")
out = ""

for line in text:
	if len(line) == 0:
		out += "\n"
		continue
	if line[0:3] == "// ":
		out += line[3:] + "\n"
	else:
		out += "// "+line+"\n"

print out[0:-1],
