#ifndef __XML_H__
#define __XML_H__

#include <libxml/parser.h>
#include <libxml/tree.h>

#include <libxml/HTMLparser.h>

enum
{
  HTML_TAG_HTML = 0,
  HTML_TAG_HEAD,
  HTML_TAG_TITLE,
  HTML_TAG_BODY,
  HTML_TAG_A,
  HTML_TAG_BR,
  HTML_TAG_P,
  HTML_TAG_IMG,
  HTML_TAG_INPUT,
  HTML_TAG_FORM,
  HTML_TAG_TEXTAREA,
  
  HTML_TAG_UNK = 1000
} HTML_TAGS;

void xml_walk_tree(xmlNode * a_node, char *url, OMS_PAGE *p);

#endif //__XML_H__