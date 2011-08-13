#!/usr/bin/env python

import os

text = os.environ['CODA_SELECTED_TEXT'].split("\n")
out = ""

for line in text:
	if len(line) == 0:
		out += "\n"
		continue
	if line[0:2] == "# ":
		out += line[2:] + "\n"
	else:
		out += "# "+line+"\n"

print out[0:-1],
