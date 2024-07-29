#include <cstdlib>

#include "cssg.h"
#include "cplusplus.h"
#include "harness.h"

void
test_cplusplus(test_batch_runner *runner)
{
    static const char md[] = "paragraph\n";
    char *html = cssg_markdown_to_html(md, sizeof(md) - 1, CSSG_OPT_DEFAULT);
    STR_EQ(runner, html, "<p>paragraph</p>\n", "libcssg works with C++");
    free(html);
}

