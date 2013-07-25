#ifndef __OMS_H__
#define __OMS_H__

#include <stdint.h> // For types like uint32_t

#define OMS_VERSION	0x3218

#pragma pack(push)
#pragma pack(1)
typedef struct
{
  uint16_t magic;
  uint32_t size;
} OMS_HEADER_COMMON;

typedef struct
{
  uint16_t res1[9];
  uint16_t tag_count;
  uint16_t part_current;
  uint16_t part_count;
  uint16_t res2;
  uint16_t Stag_count;
  uint16_t res3;
  uint8_t  res4;
  uint16_t cachable;
  uint16_t res5;
} OMS_HEADER_V2;

typedef struct
{
  uint16_t len;
  char data[0];
} OMS_STRING;
 
/*
 * Tag list
 * Stealed from OMPD
 * "I"    : Image,
 * "p"    : FormPassword,
 * "D"    : BgColor,
 * "("    : FoldOpen,
 * "T"    : Text,
 * "L"    : Link,
 * "X"    : UncompressedImage,
 * "J"    : ImagePlaceholder,
 * "K"    : ReusedImage,
 * "O"    : ReusedImage2,
 * "$"    : Dollar,
 * "+"    : BlockSeparator,
 * "A"    : Anchor,
 * "B"    : Break,
 * "C"    : SubmitFlag,
 * "E"    : LinkEnd,
 * "P"    : PhoneNumber,
 * "Q"    : EOF,
 * "R"    : HorLine,
 * "F"    : UnknownF,
 * "x"    : FormTextInput,
 * "c"    : FormCheckbox,
 * "s"    : FormSelect,
 * "o"    : FormOption,
 * "h"    : FormHidden,
 * "k"    : AuthData,
 * "V"    : Paragraph,
 * "Y"    : ReusedStyle1,
 * "y"    : ReusedStyle2,
 * "e"    : FormReset,
 * "i"    : FormImage,
 * "l"    : FormSelectEnd,
 * "u"    : FormButton,
 * "v"    : LineFeed,
 * ")"    : FoldClose,
 * "U"    : FileUpload,
 * "M"    : Alert,
 * "N"    : UnknownN,
 * "r"    : FormRadio,
 * "z"    : Indent,
 * "t"    : Unknown_t,
 * "W"    : WLink,
 * "@"    : DirectFileLink,
 * "m"    : mLink,
 * "\x08" : _8Link,
 * "\x09" : _9Link,
 * "^"    : BirdLink,
 * "Z"    : ImageDirectLink,
 * "S"    : Style,
 * "&"    : Kawai
 */

/*
 * Tags
 * Unless explicitly noted, tag is applicable to all OM formats (1.xx/2.xx/3.xx)
 */

/*
 * Auth
 */
 
// Tag 'k' - AuthCode/AuthPrefix
typedef struct
{
  uint8_t  code_type;	// 0 - AuthPrefix, 1 - AuthCode
  OMS_STRING code;
} OMS_AUTHCODE;

// Tag 'X' - image RGBA (BMP)
typedef struct
{
  uint8_t  width;
  uint8_t  height;
  uint16_t datalen;	// Width*Height*4
  uint8_t  data[0];
} OMS_TAG_X;

// Tag 'I' - image (PNG/JPEG)
typedef struct
{
  uint16_t width;
  uint16_t height;
  uint16_t datalen;
  uint16_t rsrvd;  // Pad to 8-byte boundary
  uint8_t  data[0];
} OMS_TAG_I;

// Tag 'J' - image placeholder
typedef struct
{
  uint16_t width;
  uint16_t height;
} OMS_TAG_J;

// Tag 'K' - reused image
typedef struct
{
  uint16_t width;
  uint16_t height;
  uint16_t index;
} OMS_TAG_K;

// Tag 'O' - reused image (???)
typedef struct
{
  uint16_t width;
  uint16_t height;
  uint16_t index;
} OMS_TAG_O;

/*
 * Links
 */
 
// Tag 'A' - anchor
typedef struct
{
  OMS_STRING name;
} OMS_TAG_A;

// Tag 'P' - phone number
typedef struct
{
  OMS_STRING number;
} OMS_TAG_P;

// Tag 'L' - link
typedef struct
{
  OMS_STRING url;
} OMS_TAG_L;

// Tag 'W' - Some sort of link
typedef struct
{
  OMS_STRING url;
} OMS_TAG_W;

// Tag '@' - File link
typedef struct
{
  uint16_t unk1;
  OMS_STRING url;
} OMS_TAG_AT;

// Tag 'm' - Some sort of link
typedef struct
{
  OMS_STRING url;
} OMS_TAG_m;

// Tag '\x08' - Some sort of link
typedef struct
{
  OMS_STRING url;
} OMS_TAG_08;

// Tag '\x09' - Some sort of link
typedef struct
{
  OMS_STRING url;
} OMS_TAG_09;

// Tag '^' - image link ("Bird link" in ompd?)
typedef struct
{
  OMS_STRING image_url;
} OMS_TAG_ACC;   // ^ - Circumflex accent

// Tag 'Z' - Link to image
typedef struct
{
  OMS_STRING url;   // "320x240 http://somefile.com/ulr.png" or similar
} OMS_TAG_Z;

/*
 * Style and indent
 */

// Tag 'R' - horizontal line OM 2.xx
typedef struct
{
  uint16_t r : 5, g : 6, b : 5;
} OMS_TAG_R;

// Tag 'R' - horizontal line OM 3.xx
typedef struct
{
  uint32_t r : 8, g : 8, b : 8, a : 8;
} OMS_TAG_R_V3;

// Tag 'S' - Style OM 2.xx
typedef struct
{
  uint8_t  italic:1, bold:1, underline:1, unk3:1, 
                center:1, right:1, unk6:1, unk7:1;
                
  uint16_t red:5, green:6, blue: 5;
  
  uint8_t  reserved;
} OMS_TAG_S;

// Tag 'S' - Style OM 3.xx
typedef struct
{
  uint8_t  italic:1, bold:1, underline:1, unk3:1, 
                center:1, right:1, unk6:1, unk7:1;
                
  uint32_t red:8, green:8, blue: 8, alpha:8;
  
  uint8_t  reserved;
} OMS_TAG_S_V3;

// Tag 'D' OM 2.xx - Background Color BGR565
typedef struct
{
  uint16_t red:5, green:6, blue: 5;
} OMS_TAG_D;

// Tag 'D' OM 3.xx - Background Color RGB24
typedef struct
{
  uint32_t rgb24;
} OMS_TAG_D_V3;

// Tag 'Y' - reused style
typedef struct
{
  uint8_t  index;
} OMS_TAG_Y;
 
// Tag 'y' - reused style, too
typedef struct
{
  uint16_t index;
} OMS_TAG_y;

// Tag 'z' - indent
typedef struct
{
  uint16_t indent;
} OMS_TAG_z;

/*
 * Form elements
 */

// Tag 'x' - Text input OMS 2.xx
typedef struct
{
  uint8_t  is_multiline;
  OMS_STRING id;
  OMS_STRING value;
} OMS_TAG_x;

// Tag 'x' - Text input OMS 1.xx
typedef struct
{
  OMS_STRING id;
  OMS_STRING value;
} OMS_TAG_x_V1;

// Tag 'p' - password input
typedef struct
{
  OMS_STRING id;
  OMS_STRING value;
} OMS_TAG_p;

// Tag 'c' - checkbox
typedef struct
{
  OMS_STRING id;
  OMS_STRING value;  // Usually empty
  uint8_t  checked;
} OMS_TAG_c;

// Tag 'r' - radio
typedef struct
{
  OMS_STRING id;
  OMS_STRING value;  // Usually empty
  uint8_t  checked;
} OMS_TAG_r;

// Tag 'u' - submit button
typedef struct
{
  OMS_STRING id;
  OMS_STRING value;
} OMS_TAG_u;

// Tag 'o' - option
typedef struct
{
  OMS_STRING value;
  OMS_STRING id;
  uint8_t  selected;
} OMS_TAG_o;

// Tag 's' - select
// After this tag should be tag "l" - end of select
typedef struct
{
  OMS_STRING id;
  uint8_t  unk1;
  uint16_t opts_num;
  OMS_TAG_o data[0];
} OMS_TAG_s;

// Tag 'h' - hidden element
typedef struct
{
  OMS_STRING id;
  OMS_STRING value;
} OMS_TAG_h;

// Tag 'e' - reset button
typedef struct
{
  OMS_STRING id;
  OMS_STRING value;
} OMS_TAG_e;

// Tag 'i' - image in form
typedef struct
{
  OMS_STRING id;
  OMS_STRING value;
} OMS_TAG_i;

// Tag 'U' - file upload (only in OM 3.xx)
typedef struct
{
  OMS_STRING id;
} OMS_TAG_U;

/*
 * Идентификаторы элементам формы оригинальный сервер присваивает таким образом:
 * _1001i1.u_text
 * где:
 * _1001i - неизменная часть, возможно, количество символов в поле
 * 1 - номер элемента в форме
 * .u_text - необязательная часть, задает имя элемента (атрибут name)
 * в исходной странице
 */ 

// Tag 'M' - alert message
typedef struct
{
  OMS_STRING caption;
  OMS_STRING message;
} OMS_TAG_M;
 
/*
 * Unknown tags
 */
 
// Tag 'N' - unknown
typedef struct
{
  OMS_STRING data;
} OMS_TAG_N;

// Tag 't' - unknown
typedef struct
{
  uint32_t data;
} OMS_TAG_t;

// Tag 'F' - unknown
typedef struct
{
  OMS_STRING id;
  uint16_t width;
  OMS_STRING data;
} OMS_TAG_F;

// Tag '&' - very strange tag, in ompd named "Kawai"
typedef struct
{
  uint8_t  type;
  
  union {
    struct {
      OMS_STRING unk1;
      OMS_STRING unk2;
      OMS_STRING unk3;
      OMS_STRING unk4;
      OMS_STRING unk5;
      OMS_STRING unk6;
    } unk7;
    
    uint16_t unk8[8];
  
  } data;
} OMS_TAG_AMP; // & - ampersand

/*
 * End of tags
 */

#pragma pack(pop)

typedef struct
{
  uint16_t size;
  uint16_t tag_count;
  uint8_t  *data;
} OMS_PAGE;

/*
 * Page structure: (by ya.ru)
 * +-----------------------------+
 * |      OMS_HEADER_COMMON      |
 * +-----------------------------+
 * |      ZLIB data (deflate)    |
 * | +-------------------------+ |
 * | |     OMS_HEADER_V2       | |
 * | +-------------------------+ |
 * | |  Page URL (OMS_STRING)  | |	// 1/http://ya.ru/
 * | +-------------------------+ |
 * | | AuthCode (OMS_AUTHCODE) | |
 * | +-------------------------+ |
 * | |   AuthPrefix (--//--)   | |
 * | +-------------------------+ |
 * | |     Style - tag 'S'     | |
 * | +-------------------------+ |
 * | |      Image RGBA ('X')   | |	// Favicon?
 * | +-------------------------+ |
 * | |         Tag  '+'        | |
 * | +-------------------------+ |
 * | |      Image PNG ('I')    | |	// Favicon?
 * | +-------------------------+ |
 * | |         Tag '+'         | |
 * | +-------------------------+ |
 * | |   Page title (Tag 'T')  | |	// Яндекс
 * | +-------------------------+ |
 * | |     Style - tag 'S'     | |
 * | +-------------------------+ |
 * | |          Link           | |
 * | | +---------------------+ | |
 * | | |    Tag 'L' - URL    | | |	// 0/http://m.ya.ru/mail
 * | | +---------------------+ | |
 * | | |    Tag 'T' - Text   | | |	// Войти в почту
 * | | +---------------------+ | |
 * | | |   Brake - tag 'B'   | | |
 * | | +---------------------+ | |
 * | | | Finish Ref(tag 'E') | | |
 * | | +---------------------+ | |
 * | +-------------------------+ |
 * | |     Brake - tag 'B'     | |
 * | +-------------------------+ |
 * | |        Image Link       | |
 * | | +---------------------+ | |
 * | | |    Tag 'L' - URL    | | |	// 0/http://m.yandex.ru/
 * | | +---------------------+ | |
 * | | | Tag 'J'-Image frame | | |
 * | | +---------------------+ | |
 * | | | Finish Ref(tag 'E') | | |
 * | | +---------------------+ | |
 * | +-------------------------+ |
 * | |     Brake - tag 'B'     | |
 * | +-------------------------+ |
 * | | Hidden field - tag 'h'  | |
 * | +-------------------------+ |
 * | |  Input Field - tag 'x'  | |	// _1001i0.text
 * | +-------------------------+ |
 * | |     Brake - tag 'B'     | |
 * | +-------------------------+ |
 * | |     Style - tag 'S'     | |
 * | +-------------------------+ |
 * | |          Link           | |
 * | | +---------------------+ | |
 * | | |    Tag 'L' - URL    | | |	// 0/javascript...
 * | | +---------------------+ | |
 * | | |   Brake - tag 'B'   | | |
 * | | +---------------------+ | |
 * | | |   BgColor - tag 'D' | | |
 * | | +---------------------+ | |
 * | | |    Text - tag 'T'   | | |	// Найти
 * | | +---------------------+ | |
 * | | |   Brake - tag 'B'   | | |
 * | | +---------------------+ | |
 * | | | Finish Ref(tag 'E') | | |
 * | | +---------------------+ | |
 * | +-------------------------+ |
 * | |     Brake - tag 'B'     | |
 * | +-------------------------+ |
 * | |    BgColor - tag 'D'    | |
 * | +-------------------------+ |
 * | |    Paragraph - tag 'V'  | |
 * | +-------------------------+ |
 * | |     Style - tag 'S'     | |
 * | +-------------------------+ |
 * | |      Text - tag 'T'     | |	// (c)
 * | +-------------------------+ |
 * | |          Link           | |
 * | | +---------------------+ | |
 * | | |    Tag 'L' - URL    | | |	// 0/http://yandex.ru/
 * | | +---------------------+ | |
 * | | |    Tag 'T' - Text   | | |	// Яндекс
 * | | +---------------------+ | |
 * | | |   Brake - tag 'B'   | | |
 * | | +---------------------+ | |
 * | | | Finish Ref(tag 'E') | | |
 * | | +---------------------+ | |
 * | +-------------------------+ |
 * | |  End of page - tag 'Q'  | |
 * | +-------------------------+ |
 * +-----------------------------+
 */

/* 
 * Functions
 */
OMS_PAGE *oms_new_page(void);
void oms_free_page(OMS_PAGE *p);
void oms_add_string(OMS_PAGE *p, uint8_t  *str);
void oms_add_authprefix(OMS_PAGE *p, char *prefix);
void oms_add_authcode(OMS_PAGE *p, char *code);
void oms_add_style(OMS_PAGE *p, unsigned int style);
void oms_add_text(OMS_PAGE *p, char *text);
void oms_add_link(OMS_PAGE *p, char *url, char *text);
void oms_add_break(OMS_PAGE *p);
void oms_add_paragraph(OMS_PAGE *p);
void oms_add_bgcolor(OMS_PAGE *p, char *color);
void oms_add_textcolor(OMS_PAGE *p, char *color);
void oms_add_plus(OMS_PAGE *p);

/* Формы */
void oms_add_form(OMS_PAGE *p, char *action);
void oms_add_text_input(OMS_PAGE *p, char *name, char *value);
void oms_add_pass_input(OMS_PAGE *p, char *name, char *value);
void oms_add_checkbox(OMS_PAGE *p, char *name, uint8_t  value);
void oms_add_submit(OMS_PAGE *p, char *name, char *value);

OMS_PAGE *oms_load_page(char *url);

#endif //__OMS_H__
