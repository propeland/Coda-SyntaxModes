#!/usr/bin/env python

import os

text = os.environ['CODA_SELECTED_TEXT']

if (text[0:2] == "/*" and text[-2:] == "*/"):
	text = text[2:-2]
else:
	text = "/*"+text+"*/"

print text