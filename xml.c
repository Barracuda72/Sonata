#include <stdio.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include <oms.h>
#include <xml.h>

#define SCMP(x, y) strcasecmp(x, y)

static int xml_get_tag_id(char *tag)
{
  if (!SCMP(tag, "html"))
    return HTML_TAG_HTML;
  else if (!SCMP(tag, "head"))
    return HTML_TAG_HEAD;
  else if (!SCMP(tag, "title"))
    return HTML_TAG_TITLE;
  else if (!SCMP(tag, "body"))
    return HTML_TAG_BODY;
  else if (!SCMP(tag, "a"))
    return HTML_TAG_A;
  else if (!SCMP(tag, "br"))
    return HTML_TAG_BR;
  else if (!SCMP(tag, "p"))
    return HTML_TAG_P;
  else if (!SCMP(tag, "img"))
    return HTML_TAG_IMG;
  else if (!SCMP(tag, "input"))
    return HTML_TAG_INPUT;
  else if (!SCMP(tag, "form"))
    return HTML_TAG_FORM;
  else if (!SCMP(tag, "textarea"))
    return HTML_TAG_TEXTAREA;
    
  return HTML_TAG_UNK;
}

char *xml_get_attr(xmlNode * a_node, char *attr)
{
  xmlAttr *cur_attr = NULL;
	for (cur_attr = a_node->properties; cur_attr; cur_attr = cur_attr->next)
		{
		  if ((cur_attr->type == XML_ATTRIBUTE_NODE) && !SCMP(cur_attr->name, attr)) {
	    //printf("Attribute %s, value %s\n", cur_attr->name, cur_attr->children->content);
      return cur_attr->children->content;
	  }
  }
  return 0;
}

#define XML_TEXT_MAGIC 0xFEAD 

/*
 * Ищет первый текстовый узел среди потомков
 */
xmlNode *xml_find_text_node(xmlNode * a_node)
{
  xmlNode *ch_node = NULL;
  xmlNode *x_node = NULL;
  
  for (ch_node = a_node->children; 
      (ch_node != NULL) && (ch_node->type != XML_TEXT_NODE) && (ch_node->extra != XML_TEXT_MAGIC);
      ch_node = ch_node->next)
  {
    x_node = xml_find_text_node(ch_node);
    if (x_node)
      return x_node;
  }
  return ch_node;
}

void xml_walk_tree(xmlNode * a_node, char *url, OMS_PAGE *p)
{
  xmlNode *cur_node = NULL;
  xmlNode *ch_node = NULL;
	xmlAttr *cur_attr = NULL;
  
  char *i;
  int j;
  
  for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
    if (cur_node->type == XML_ELEMENT_NODE) {
      //printf("Tag: %s\n", cur_node->name);
      
      char *l, *link, *name;
      switch(xml_get_tag_id(cur_node->name))
      {
        case HTML_TAG_HTML:
        case HTML_TAG_HEAD:
          break;

        case HTML_TAG_TITLE:
          name = "Page title";
          ch_node = xml_find_text_node(cur_node);
          if (ch_node)
          {
            // Dirty hack
            ch_node->extra = XML_TEXT_MAGIC;
            oms_add_plus(p);
            oms_add_text(p, ch_node->content);
          }
          break;
          
        case HTML_TAG_BODY:
          l = xml_get_attr(cur_node, "bgcolor");
          if (l) oms_add_bgcolor(p, l);
          l = xml_get_attr(cur_node, "text");
          if (l) oms_add_textcolor(p, l);
          break;
          
        case HTML_TAG_BR:
          oms_add_break(p);
          break;
          
        case HTML_TAG_P:
          oms_add_paragraph(p);
          break;
          
        case HTML_TAG_A:
          //printf("z");
          l = xml_get_attr(cur_node, "href");
          if (l != 0)
          {
            link = malloc(strlen(l) + 10 + strlen(url));
            if(!strstr(l, "://")) // Нет протокола -> ссылка неполная (не всегда так, но пох)
            { 
              if (l[0] == '/')  // Путь от корня сайта
                i = strchr(url+7, '/');
              else              // Относительно текущей директории
              {
                i = strrchr(url, '/');
                if (i == (url+6)) // А нету в конце слеша
                  i = NULL;
              }
              
              if (i != NULL)
                *i = 0;
                
              sprintf(link, "0/%s/%s", url, l);
              
              if (i != NULL)
                *i = '/';
            }
            else
              sprintf(link, "0/%s", l);
          } else {
            link = strcpy(malloc(12), "error:link");
          }
          name = "Link";
          //printf("e");
          ch_node = xml_find_text_node(cur_node);

          if (ch_node)
          {
            // Dirty hack
            ch_node->extra = XML_TEXT_MAGIC;
            name = ch_node->content;
          }
          oms_add_link(p, link, name);
          free(link);
          //printf("q");
          break;
          
        case HTML_TAG_IMG:
          oms_add_text(p, "[Img]");
          break;
        
        case HTML_TAG_FORM:
          link = xml_get_attr(cur_node, "action");
          oms_add_form(p, link);
          break;
        
        case HTML_TAG_TEXTAREA:
          // HACK for google.com and similar
          name = xml_get_attr(cur_node, "style");
          if (!name || !strstr(name, "display:none"))
          {
            name = xml_get_attr(cur_node, "name");
            if (!name) name = "dname";
            link = xml_get_attr(cur_node, "value");
            if (!link) link = "";
            oms_add_text_input(p, name, link);
          }
          break;
          
        case HTML_TAG_INPUT:
          l = xml_get_attr(cur_node, "type");
          if (!l) l = "text";
          name = xml_get_attr(cur_node, "name");
          if (!name) name = "dname";
          link = xml_get_attr(cur_node, "value");
          if (!link) link = "";
          
          if (!SCMP(l, "text"))
            oms_add_text_input(p, name, link);
          else if (!SCMP(l, "password"))
            oms_add_pass_input(p, name, link);
          else if (!SCMP(l, "submit"))
            oms_add_submit(p, name, link);
          else if (!SCMP(l, "checkbox"))
          {
            link = xml_get_attr(cur_node, "checked");
            j = 0;
            if (link && (!SCMP(link, "true")))
              j = 1;
            oms_add_checkbox(p, name, 1);
          }
          break;
          
        default:
          break;
      }
      /*
			for (cur_attr = cur_node->properties; cur_attr; cur_attr = cur_attr->next)
			{
			  if (cur_attr->type == XML_ATTRIBUTE_NODE) {
			    printf("Attribute %s, value %s\n", cur_attr->name, cur_attr->children->content);
			  }
			}*/
    } else if ((cur_node->type == XML_TEXT_NODE) && (cur_node->extra != XML_TEXT_MAGIC)) {
		  //printf("Text node %s\n", cur_node->content);
		  oms_add_text(p, cur_node->content);
		}

    xml_walk_tree(cur_node->children, url, p);
  //  printf("Boo!\n");
  }
}
#if 0
int main(int argc, char *argv[])
{
    xmlDoc *doc = NULL;
    xmlNode *root_element = NULL;

    /*
     * this initialize the library and check potential ABI mismatches
     * between the version it was compiled for and the actual shared
     * library used.
     */
    LIBXML_TEST_VERSION

    /*parse the file and get the DOM */
    doc = xmlReadFile("test.html", NULL, 0);

    if (doc == NULL) {
        printf("error: could not parse file %s\n", argv[1]);
    }

    /*Get the root element node */
    root_element = xmlDocGetRootElement(doc);

    OMS_PAGE *p = oms_new_page();

	oms_add_string(p, "1/http://ya.ru/");
    oms_add_authcode(p, "c37c206d2c235978d086b64c39a2fc17df68dbdd5dc04dd8b199177f95be6181");
    oms_add_authprefix(p, "t19-12");
    oms_add_style(p, 0x02000002);
	
    walk_tree(root_element, p);
	oms_finalize_page(p);
    oms_free_page(p);

    /*free the document */
    xmlFreeDoc(doc);

    /*
     *Free the global variables that may
     *have been allocated by the parser.
     */
    xmlCleanupParser();

    return 0;
}
#endif