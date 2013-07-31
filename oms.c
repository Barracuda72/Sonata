#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/nanohttp.h>
#include <zlib.h>

#include <oms.h>
#include <xml.h>
#include <z_def.h>

OMS_PAGE *oms_new_page(void)
{
  OMS_PAGE *p = malloc(sizeof(OMS_PAGE));
  memset(p, 0, sizeof(OMS_PAGE));
  return p;
}

void oms_free_page(OMS_PAGE *p)
{
  if (p != NULL)
  {
    if (p->data != NULL)
      free(p->data);
    free(p);
  }
}

static void oms_add_data(OMS_PAGE *p, unsigned char *data, int size)
{
  p->data = realloc(p->data, p->size + size);
  memcpy(&p->data[p->size], data, size);
  p->size += size;
}

static void oms_add_data_r(OMS_PAGE *p, unsigned char *data, int size)
{
  p->data = realloc(p->data, p->size + size);
  for (; size > 0; size--)
  {
    p->data[p->size] = data[size-1];
    p->size++;
  }
}

static void oms_add_tag(OMS_PAGE *p, unsigned char *tag)
{
  oms_add_data(p, tag, 1);
  p->tag_count++;
}

void oms_add_string(OMS_PAGE *p, unsigned char *str)
{
  unsigned short len[1];
  len[0] = strlen(str);
  oms_add_data_r(p, len, 2);
  oms_add_data(p, str, len[0]);
}

static void oms_add_authdata(OMS_PAGE *p, char *authdata, unsigned char type)
{
  unsigned char data[1];
  data[0] = type;
  oms_add_tag(p, "k");
  oms_add_data(p, data, 1);
  oms_add_string(p, authdata);
}

void oms_add_authprefix(OMS_PAGE *p, char *prefix)
{
  oms_add_authdata(p, prefix, 0);
}

void oms_add_authcode(OMS_PAGE *p, char *code)
{
  oms_add_authdata(p, code, 1);
}

// Ну да, я лентяй
static unsigned char oms_ch(unsigned char c)
{
  if((c >= '0') && (c <= '9'))
    return c - '0';
    
  if((c >= 'A') && (c <= 'F'))
    return c - 'A' + 10;
    
  if((c >= 'a') && (c <= 'f'))
    return c - 'a' + 10;

  return 0;
}

static unsigned short oms_calc_color(char *color)
{
  unsigned char r,g,b;
  
  if (!color)
    return;
    
  if (strlen(color) < 7)
    return;
    
  if (color[0] != '#') // имена цветов пока не обрабатываем
    return; 
    
  r = (oms_ch(color[2])|(oms_ch(color[1])<<4))>>3;
  g = (oms_ch(color[4])|(oms_ch(color[3])<<4))>>2;
  b = (oms_ch(color[6])|(oms_ch(color[5])<<4))>>3;

  return (r)|(g<<5)|(b<<11);
}

void oms_add_textcolor(OMS_PAGE *p, char *color)
{
  oms_add_style(p, oms_calc_color(color)<<8);
}

void oms_add_style(OMS_PAGE *p, unsigned int style)
{
  unsigned int st[1];
  st[0] = style;
  oms_add_tag(p, "S");
  oms_add_data_r(p, st, 4);
}

void oms_add_text(OMS_PAGE *p, char *text)
{
  /* Ебучий костыль */
  while ((*text == '\r') || (*text == '\n')) text++;
  if (*text == 0)
    return;
  oms_add_tag(p, "T");
  oms_add_string(p, text);
}

void oms_add_break(OMS_PAGE *p)
{
  oms_add_tag(p, "B");
  //oms_add_tag(p, "E");
}

void oms_add_link(OMS_PAGE *p, char *url, char *text)
{
  oms_add_tag(p, "L");
  oms_add_string(p, url);
  oms_add_text(p, text);
  oms_add_break(p);
  oms_add_tag(p, "E");
}

void oms_add_bgcolor(OMS_PAGE *p, char *color)
{
  if (!color) return;
  unsigned short cl[2];
  cl[0] = oms_calc_color(color);
  oms_add_tag(p, "D");
  oms_add_data_r(p, cl, 2);
}

void oms_add_plus(OMS_PAGE *p)
{
  oms_add_tag(p, "+");
}

void oms_add_paragraph(OMS_PAGE *p)
{
  oms_add_tag(p, "V");
}

static void oms_finalize_page(OMS_PAGE *p)
{
  OMS_HEADER_V2 v2h;
  z_stream strm;
  int ret;
  
  oms_add_tag(p, "Q");
  unsigned char *data = malloc(p->size + sizeof(OMS_HEADER_COMMON) + sizeof(OMS_HEADER_V2));
  
  OMS_HEADER_COMMON *ch = (OMS_HEADER_COMMON *)data;
  ch->magic = OMS_VERSION;
  
  memset(v2h.res1, 0, 9);
  v2h.tag_count = ((p->tag_count&0xFF)<<8)|(p->tag_count>>8);
  v2h.part_current = 0x0100;
  v2h.part_count = 0x0100;
  v2h.res2 = 0;
  v2h.Stag_count = 0x0400;
  v2h.res3 = 0;
  v2h.res4 = 0;
  v2h.cachable = 0xFFFF;
  v2h.res5 = 0;
  
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;

  ret = deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY);
  if (ret != Z_OK)
  {
    zerr(ret);
    return;
  }
  
  strm.avail_in = sizeof(OMS_HEADER_V2);
  strm.next_in = (unsigned char *)&v2h;
  strm.avail_out = p->size + sizeof(OMS_HEADER_V2);
  strm.next_out = data+sizeof(OMS_HEADER_COMMON);
  ret = deflate(&strm, Z_NO_FLUSH);
  assert(ret != Z_STREAM_ERROR);
  
  strm.next_in = p->data;
  strm.avail_in = p->size;
  ret = deflate(&strm, Z_FINISH);
  assert(ret != Z_STREAM_ERROR);
  free(p->data);
  
  p->data = data;
  //p->size = (sizeof(OMS_HEADER_V2) + p->size) -  strm.avail_out;
  p->size = (sizeof(OMS_HEADER_COMMON) + sizeof(OMS_HEADER_V2) + p->size);
  ch->size = (p->size>>24) | ((p->size>>8)&0xFF00) | ((p->size<<8)&0xFF0000) | (p->size<<24);
  deflateEnd(&strm);
}

static void oms_add_header(OMS_PAGE *p)
{
  oms_add_string(p, "1/internal:error");
  oms_add_authcode(p, "c37c206d2c235978d086b64c39a2fc17df68dbdd5dc04dd8b199177f95be6181");
  oms_add_authprefix(p, "t19-12");
  oms_add_style(p, 0x02000002);
  oms_add_plus(p);
  oms_add_text(p, "Internal server error");
}

static OMS_PAGE *oms_error(char *url, char *reason)
{
  OMS_PAGE *p = oms_new_page();
  oms_add_header(p);
  oms_add_text(p, reason);
  oms_add_break(p);
  oms_add_text(p, url);
  oms_finalize_page(p);
  return p;
}

OMS_PAGE *oms_load_page(char *o_url)
{
  char *url = malloc(strlen(o_url) + 8);
  OMS_PAGE *p = NULL;
  
  /* nanoHTTP не поддерживает https */
  
  if (!strncasecmp(o_url, "https://", 8))
    o_url += 8;
  
  /* проверка адреса */
  if (strncasecmp(o_url, "http://", 7) != 0)
  {
    sprintf(url, "http://%s", o_url);
  } else {
    strcpy(url, o_url);
  }

  printf("URL is %s\n", url);
  
  htmlDocPtr doc = NULL;
  htmlNodePtr root_element = NULL;
  
  /* Убираем все лишние пробелы */
  xmlKeepBlanksDefault(0);
  
  /* parse the file and get the DOM */
  xmlNanoHTTPInit();
  void *h = xmlNanoHTTPOpen(url, NULL);
  int i = 0;
  int j = 0;
  
  //printf("h = %d\n", h);
  
  char *buf = NULL;
  int code = xmlNanoHTTPReturnCode(h);
    
  if (code != 200)
  {
    p = oms_error(url, "Timeout loading page");
    xmlNanoHTTPClose(h);
    free(url);
    return p;
  }
    
  while((i = xmlNanoHTTPRead(h, (buf = realloc(buf, j+1024)) + j, 1024)) > 0) j += i;
  
  //printf("Encoding: %s\n", xmlNanoHTTPEncoding(h));
  //printf("Recvd: %d\n", j);
  
  //doc = htmlReadFile(url, "UTF-8", XML_PARSE_NOERROR | XML_PARSE_NOWARNING);
  char *encoding = xmlNanoHTTPEncoding(h);
  if (!encoding) encoding = "UTF-8";
  
  doc = htmlParseDoc(buf, encoding);
  xmlNanoHTTPClose(h);
  //printf("Almost done\n");
  
  if (doc == NULL) {
    printf("error: could not parse file %s\n", url);
    p = oms_error(url, "Internal server error while parsing ");
    free(url);
    return p;
  }
  
  /*Get the root element node */
  root_element = xmlDocGetRootElement(doc);

  p = oms_new_page();
  
  oms_add_string(p, o_url);
  oms_add_authcode(p, "c37c206d2c235978d086b64c39a2fc17df68dbdd5dc04dd8b199177f95be6181");
  oms_add_authprefix(p, "t19-12");
  oms_add_style(p, 0x02000002);

  //printf("Walking tree\n");
  
  // Вырезаем нахер из URL GET-данные (они нам больше не нужны)
  char *get = strchr(url, '?');
  if (get)
    *get = 0;
  
  xml_walk_tree(root_element, url, p);
  free(url);
  
#if 0
  FILE *f = fopen("test.oms", "wb");
  fwrite(p->data, p->size, 1, f);
  fclose(f);
#endif  

  oms_finalize_page(p);
  
  xmlFreeDoc(doc);
  xmlCleanupParser();
  
  return p;
}

/* Формы */
void oms_add_form(OMS_PAGE *p, char *action)
{
  oms_add_tag(p, "h");
  //oms_add_string(p, "opf");
  if (action == NULL)
    oms_add_string(p, "1");
  else
    oms_add_string(p, action);
    
  oms_add_string(p, "1");
}

void oms_add_text_input(OMS_PAGE *p, char *name, char *value)
{
  char tmp[1];
  tmp[0] = 0;
  
  oms_add_tag(p, "x");
  oms_add_data(p, tmp, 1);
  oms_add_string(p, name);
  oms_add_string(p, value);
}

void oms_add_pass_input(OMS_PAGE *p, char *name, char *value)
{
  oms_add_tag(p, "p");
  oms_add_string(p, name);
  oms_add_string(p, value);
}

void oms_add_checkbox(OMS_PAGE *p, char *name, unsigned char value)
{
  unsigned short tmp[1];
  tmp[0] = 0;
  unsigned char chkd[1];
  chkd[0] = value;
  
  oms_add_string(p, name);
  oms_add_tag(p, "c");
  oms_add_data(p, tmp, 2);
  oms_add_data(p, chkd, 1);
}

void oms_add_submit(OMS_PAGE *p, char *name, char *value)
{
  oms_add_tag(p, "u");
  oms_add_string(p, name);
  oms_add_string(p, value);
}
