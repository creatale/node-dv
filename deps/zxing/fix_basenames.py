#!/usr/bin/env python
#
# Fix duplicated source basenames by prepending a number.
#
import os

# Collect source filenames, grouped by basename.
sources = {}
for root, dirnames, filenames in os.walk('core'):
    for filename in filenames:
        if filename.endswith('.cpp'):
            if not filename in sources:
                sources[filename] = [ os.path.join(root, filename) ]
            else:
                sources[filename].append(os.path.join(root, filename))

# Rename duplicates.
for basename, filenames in sources.items():
    if len(filenames) <= 1:
        continue
    print basename
    index = 0
    for filename in filenames:
        index += 1
        new_filename = os.path.join(os.path.dirname(filename),
            str(index) + basename)
        print("  " + new_filename)
        os.rename(filename, new_filename)
