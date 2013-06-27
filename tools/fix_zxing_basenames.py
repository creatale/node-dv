#!/usr/bin/env python
#
# Fix duplicated source basenames by prepending a number.
#
import os
from optparse import OptionParser

# Parse command line options.
parser = OptionParser()
parser.add_option('-a', '--apply', action='store_const', const='apply',
                  dest='mode', help='applies basename fix')
parser.add_option('-r', '--remove', action='store_const', const='remove',
                  dest='mode', help='removes basename fix')
parser.add_option('--dry-run',  action='store_true',
                  dest='dry', default=False,
                  help='just print what would happen.')
(options, args) = parser.parse_args()

def rename(filename, new_filename):
    print(' mv ' + filename + '\n    ' + new_filename)
    if not options.dry:
        os.rename(filename, new_filename)

def applyFix():
    # Collect source filenames, grouped by basename.
    sources = {}
    for root, dirnames, filenames in os.walk('deps/zxing/core'):
        for filename in filenames:
            if filename.endswith('.cpp'):
                if not filename in sources:
                    sources[filename] = [os.path.join(root, filename)]
                else:
                    sources[filename].append(os.path.join(root, filename))
    for basename, filenames in sources.items():
        if len(filenames) <= 1:
            continue
        # Apply number prefix to duplicates.
        index = 0
        for filename in filenames:
            index += 1
            new_filename = os.path.join(os.path.dirname(filename),
                str(index) + basename)
            rename(filename, new_filename)

def removeFix():
    for root, dirnames, filenames in os.walk('deps/zxing/core'):
        for filename in filenames:
            if filename.endswith('.cpp') and filename[0].isdigit():
                # Remove number prefix.
                path = os.path.join(root, filename)
                new_path = os.path.join(root, filename.lstrip('0123456789'))
                rename(path, new_path)

if __name__ == '__main__':
    if options.mode is 'apply':
        applyFix()
    elif options.mode is 'remove':
        removeFix()
    else:
        parser.print_help()
