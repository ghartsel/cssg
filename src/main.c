#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cssg.h"
#include "node.h"

#if defined(_WIN32) && !defined(__CYGWIN__)
#include <io.h>
#include <fcntl.h>
#endif

#include "toml.h"

typedef enum {
  FORMAT_NONE,
  FORMAT_HTML,
  FORMAT_XML,
  FORMAT_MAN,
  FORMAT_COMMONMARK,
} writer_format;

#define MAX_FILENAME_LENGTH 256
#define MAX_FILES 100

const char *testFiles[2] = {
  "short-sample.md",
  "test.md"
};

void print_usage(void) {
  printf("Usage:   cssg [FILE*]\n");
  printf("Options:\n");
  printf("  --to, -t FORMAT  Specify output format (html, xml, man, commonmark)\n");
  printf("  --sourcepos      Include source position attribute\n");
  printf("  --hardbreaks     Treat newlines as hard line breaks\n");
  printf("  --nobreaks       Render soft line breaks as spaces\n");
  printf("  --safe           Omit raw HTML and dangerous URLs\n");
  printf("  --unsafe         Render raw HTML and dangerous URLs\n");
  printf("  --smart          Use smart punctuation\n");
  printf("  --validate-utf8  Replace invalid UTF-8 sequences with U+FFFD\n");
  printf("  --help, -h       Print usage information\n");
  printf("  --version        Print version\n");
}

static void render_topic(cssg_node *document, writer_format writer, int options) {
  char *result;

  switch (writer) {
  case FORMAT_HTML:
    result = cssg_render_html(document, options);
    break;
  case FORMAT_XML:
    result = cssg_render_xml(document, options);
    break;
  case FORMAT_MAN:
    result = cssg_render_man(document, options, 0);
    break;
  case FORMAT_COMMONMARK:
    result = cssg_render_commonmark(document, options, 0);
    break;
  default:
    fprintf(stderr, "Unknown format %d\n", writer);
    exit(1);
  }
  fwrite(result, strlen(result), 1, stdout);
  document->mem->free(result);
}

int main(int argc, char *argv[]) {
  FILE *testFiles;
  char fList[MAX_FILES][MAX_FILENAME_LENGTH];
  int fileCount = 0;
  int *files;
  char buffer[8192];
  cssg_parser *parser;
  size_t bytes;
  cssg_node *document;
  int options = CSSG_OPT_DEFAULT;

#if defined(_WIN32) && !defined(__CYGWIN__)
  _setmode(_fileno(stdin), _O_BINARY);
  _setmode(_fileno(stdout), _O_BINARY);
#endif

  testFiles = fopen("iaList.txt", "r");
  if (testFiles == NULL) {
    fprintf(stderr, "Error opening file %s: %s %s\n", "iaList.txt", strerror(errno), argv[1]);
    exit(1);
  }

  while (fgets(fList[fileCount], MAX_FILENAME_LENGTH, testFiles) != NULL && fileCount < MAX_FILES) {
    // Remove the newline character from the filename, if present
    fList[fileCount][strcspn(fList[fileCount], "\n")] = '\0';
    fileCount++;
  }

  // Close the list file
  fclose(testFiles);

  for (int i = 0; i < fileCount; i++) {
    files = (int *)calloc(argc, sizeof(*files));

    parser = cssg_parser_new(options);

    char path[MAX_FILENAME_LENGTH] = "";
    strcat(path, "topics/");
    strcat(path, fList[i]);
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
      fprintf(stderr, "Error opening file %s: %s\n", fList[i], strerror(errno));
      exit(1);
    }

    while ((bytes = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
      cssg_parser_feed(parser, buffer, bytes);
      if (bytes < sizeof(buffer)) {
        break;
      }
    }

    fclose(fp);

    document = cssg_parser_finish(parser);
    cssg_parser_free(parser);

    // writer options: FORMAT_MAN, FORMAT_HTML, FORMAT_XML, FORMAT_COMMONMARK
    render_topic(document, FORMAT_HTML, options);

    cssg_node_free(document);

    free(files);
  }
  return 0;
}
