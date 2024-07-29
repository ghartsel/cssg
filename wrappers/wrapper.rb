#!/usr/bin/env ruby
require 'ffi'

module Cssg
  extend FFI::Library

  class Mem < FFI::Struct
    layout :calloc, :pointer,
           :realloc, :pointer,
           :free, callback([:pointer], :void)
  end

  ffi_lib ['libcssg', 'cssg']
  attach_function :cssg_get_default_mem_allocator, [], :pointer
  attach_function :cssg_markdown_to_html, [:string, :size_t, :int], :pointer
end

def markdown_to_html(s)
  len = s.bytesize
  cstring = Cssg::cssg_markdown_to_html(s, len, 0)
  result = cstring.get_string(0)
  mem = Cssg::cssg_get_default_mem_allocator
  Cssg::Mem.new(mem)[:free].call(cstring)
  result
end

STDOUT.write(markdown_to_html(ARGF.read()))
