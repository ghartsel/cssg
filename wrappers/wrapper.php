<?php

function markdownToHtml(string $markdown): string
{
    $ffi = FFI::cdef(
        'char *cssg_markdown_to_html(const char *text, size_t len, int options);',
        'libcssg.so'
    );

    $pointerReturn = $ffi->cssg_markdown_to_html($markdown, strlen($markdown), 0);
    $html = FFI::string($pointerReturn);
    FFI::free($pointerReturn);

    return $html;
}

$markdown = <<<'md'
# First level title

## Second level title

Paragraph

md;

echo markdownToHtml($markdown) . PHP_EOL;
