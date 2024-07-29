#!/usr/bin/env python

# Example for using the shared library from python
# Will work with either python 2 or python 3
# Requires cssg library to be installed

from ctypes import *
import sys
import platform

sysname = platform.system()

if sysname == 'Darwin':
    libname = "libcssg.dylib"
elif sysname == 'Windows':
    libname = "cssg.dll"
else:
    libname = "libcssg.so"
cssg = CDLL(libname)

class cssg_mem(Structure):
    _fields_ = [("calloc", c_void_p),
                ("realloc", c_void_p),
                ("free", CFUNCTYPE(None, c_void_p))]

get_alloc = cssg.cssg_get_default_mem_allocator
get_alloc.restype = POINTER(cssg_mem)
free_func = get_alloc().contents.free

markdown = cssg.cssg_markdown_to_html
markdown.restype = POINTER(c_char)
markdown.argtypes = [c_char_p, c_size_t, c_int]

opts = 0 # defaults

def md2html(text):
    text = text.encode('utf-8')
    cstring = markdown(text, len(text), opts)
    result = string_at(cstring).decode('utf-8')
    free_func(cstring)
    return result

sys.stdout.write(md2html(sys.stdin.read()))
