#ifndef CSSG_HTML_H
#define CSSG_HTML_H

#ifdef __cplusplus
extern "C" {
#endif

char *htmlOpen =
    "<!doctype html>\n\
<html lang=\"en\">\n";

char *htmlClose =
    "</body>\n\
</html>";

char *htmlHead =
    "<head>\n\
<meta charset=\"utf-8\">\n\
<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n\
<meta name=\"description\" content=\"VARIABLE\">\n\
<meta name=\"generator\" content=\"cssg 0.0.1\">\n\
<title>DVARIABLE</title>\n\
<link rel=\"canonical\" href=\"https://docs.fauna.com/fauna/current/get_started/index.html\">\n\
<link rel=\"stylesheet\" href=\"../theme/css/cssg.css\">\n\
<link rel=\"icon\" href=\"../theme/images/favicon.svg\">\n\
<script src=\"../theme/js/cssg.js\"></script>\n\
</head>\n";

char *bodyOpen =
    "<body class=\"topic\"><a name=\"top\"></a>\n\
<div class=\"body-wrapper\">\n";

char *nav =
    "<nav id=\"sidebar\">\n\
</nav>\n";

char *mainHTML =
    "<main id=\"content\">\n";

char *article =
    "<article>\n";

char *headerArticle =
    "<header>\n\
</header>\n";

char *asideArticle =
    "<aside>\n\
</aside>\n";

char *mainArticle =
    "<main id=\"topic-body\">";

char *htmlFooter =
    "</main>\n\
</article>\n\
<footer>\n\
<div class=\"footer\" role=\"contentinfo\">\n\
&#169; Copyright VARIABLE, VARIABLE\n\
</div>\n\
</footer>";

char *htmlTerminal =
    "</main>\n\
</div>\n\
</body>\n\
</html>\n";

#ifdef __cplusplus
}
#endif

#endif
