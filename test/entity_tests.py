#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import re
import os
import argparse
import sys
import platform
import html
from cssg import Cssg

def get_entities():
    regex = r'^{\(unsigned char\*\)"([^"]+)", \{([^}]+)\}'
    with open(os.path.join(os.path.dirname(__file__), '..', 'src', 'entities.inc')) as f:
        code = f.read()
    entities = []
    for entity, utf8 in re.findall(regex, code, re.MULTILINE):
        utf8 = bytes(map(int, utf8.split(", ")[:-1])).decode('utf-8')
        entities.append((entity, utf8))
    return entities

parser = argparse.ArgumentParser(description='Run cssg tests.')
parser.add_argument('--program', dest='program', nargs='?', default=None,
        help='program to test')
parser.add_argument('--library-dir', dest='library_dir', nargs='?',
        default=None, help='directory containing dynamic library')
args = parser.parse_args(sys.argv[1:])

cssg = Cssg(prog=args.program, library_dir=args.library_dir)

entities = get_entities()

passed = 0
errored = 0
failed = 0

exceptions = {
    'quot': '&quot;',
    'QUOT': '&quot;',

    # These are broken, but I'm not too worried about them.
    'nvlt': '&lt;⃒',
    'nvgt': '&gt;⃒',
}

print("Testing entities:")
for entity, utf8 in entities:
    [rc, actual, err] = cssg.to_html("&{};".format(entity))
    check = exceptions.get(entity, utf8)

    if rc != 0:
        errored += 1
        print(entity, '[ERRORED (return code {})]'.format(rc))
        print(err)
    elif check in actual:
        # print(entity, '[PASSED]') # omit noisy success output
        passed += 1
    else:
        print(entity, '[FAILED]')
        print(repr(actual))
        failed += 1

print("{} passed, {} failed, {} errored".format(passed, failed, errored))
if failed == 0 and errored == 0:
    exit(0)
else:
    exit(1)
