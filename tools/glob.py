#!/usr/bin/env python
#
# Script to generate a list of source files.
#
import fnmatch
import sys
import os

matches = []
for root, dirnames, filenames in os.walk(sys.argv[1]):
	for filename in fnmatch.filter(filenames, '*.c') \
		+ fnmatch.filter(filenames, '*.cpp') \
		+ fnmatch.filter(filenames, '*.cc'):
		matches.append(os.path.normpath(os.path.join(root, filename)).replace('\\','/'))

for match in sorted(matches):
	print "        '" + match + "',"
	