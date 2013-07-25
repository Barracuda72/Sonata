#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <zlib.h>
#include <libxml/parser.h>
#include <signal.h>

#include <z_def.h>
#include <oms.h>

#include <version.h>

#define SON_STACKSIZE 0x10000
#define SON_TIMEOUT 300

/*
#ifdef WIN32
#define SON_WIN32 // Для теста под виндой, пока не нужно - я пользую Cygwin
#endif
*/

int mainSock = 0;
int finished = 0;

struct sockaddr_in serv;
struct sockaddr_in client;

typedef struct
{
  unsigned int sock;
  struct sockaddr_in addr;
} conn_info;

#ifdef SON_WIN32
// Win-specific
WSADATA WSAData = {0};
typedef int socklen_t;
#define GET_ERROR() WSAGetLastError()
#define HANDLER unsigned int __stdcall
#define IOCTL(x, y, z) ioctlsocket(x, y, (u_long *)z)
#define TIME() _time64(0)
#define SHUT_RDWR 2
#define snprintf _snprintf_c
#define CLOSE(x) closesocket(x)

#else
// Unix-specific
#define GET_ERROR() errno
#define HANDLER void *
#define IOCTL(x, y, z) ioctl(x, y, z)
#define TIME() time(0)
#define CLOSE(x) close(x)

void sigpipe_handler(int a)
{
  printf("Caught SIGPIPE, probably some socket fucked-up\n");
  /*return */signal(SIGPIPE, sigpipe_handler);
}

void sigint_handler(int a)
{
  printf("Caught SIGINT, exiting... Hit Ctrl-C once more to terminate immediately\n");
  finished = 1;
  /*return*/ signal(SIGINT, 0);
}

pthread_attr_t pth_attr;
pthread_t thread;
#endif

HANDLER handle_connection(void *p);

char *url_decode(char *url);

/*
 * Для libxml
 */
 
void _nullGenericErrorFunc(void* ctxt, char* msg, ...)
{
}

void _initThreadLogging()
{
  xmlThrDefSetGenericErrorFunc(NULL, _nullGenericErrorFunc);
  xmlSetGenericErrorFunc(NULL, _nullGenericErrorFunc);
}

int main(int argc, char *argv[])
{
  int port = 8008;
  
  // Вдруг нам передали порт
  if(argc > 1)
  {
    port = atoi(argv[1]);
  
    // Если порт допустим
    if((port < 2) && (port > 65535))
    {
      printf("Incorrect port number. Value must be in range 2-65535\n");
      return -1;
    }
  }

  printf("Sonata - an alternate OperaMini 2.xx server (ver %s)\n", SON_VERSION);
#ifdef SON_WIN32
  printf("Win32 build\n");
#else
  printf("Linux %s build\n", (sizeof(long) == 4 ? "32-bit" : "64-bit"));
  printf("Thread stack size: %u\n", SON_STACKSIZE);
#endif

  int seed;
#ifdef SON_WIN32
  seed = GetTickCount();
#else
  seed = time(0);
#endif

  srand(seed);

  // Инициализируем libxml
  LIBXML_TEST_VERSION

  _initThreadLogging();
  
#ifdef SON_WIN32
  if ( WSAStartup(MAKEWORD(2, 0), &WSAData) )
  {
    printf("WSAStartup() failed with error: %i\n", WSAGetLastError());
    return -1;
  } else 
#else
  setvbuf((FILE *)stdout, 0, 2, 0);
#endif
  {
    // Создаем сокет
    mainSock = socket(AF_INET, SOCK_STREAM, 0);
    if(mainSock == -1)
    {
      printf("socket() failed with error: %i\n", GET_ERROR());
      return -1;
    } else {
      // Запрашиваем возможность использования занятого сокета
      int optval = 1;
      /*if ( setsockopt(mainSock, SOL_SOCKET, SO_REUSEADDR, (const char *)&optval, 4) == -1 )
      {
        printf("setsockopt(SO_REUSEADDR) failed with error: %i\n", GET_ERROR());
        return -1;
      } else*/ {
        serv.sin_addr.s_addr = INADDR_ANY;
        serv.sin_family = AF_INET;
        serv.sin_port = htons(port);

        if ( bind(mainSock, (const struct sockaddr *)&serv, sizeof(struct sockaddr)) == -1 )
        {
          printf("bind() failed with error: %i\n", GET_ERROR());
          return -1;
        } else {
          if ( listen(mainSock, 5) == -1 )
          {
            printf("listen() failed with error: %i\n", GET_ERROR());
            return -1;
          } else {
            socklen_t namelen = 16;

#ifndef SON_WIN32
            signal(SIGINT, sigint_handler);
            signal(SIGPIPE, sigpipe_handler);
            while ( !finished )
#else
            while ( 1 )
#endif
            {
              socklen_t cl_len = sizeof(struct sockaddr);
              int newSock = accept(mainSock, (struct sockaddr *)&client, &cl_len);
              if ( newSock == -1 )
              {
                printf("Socket error. Exit! errno=%d\n", GET_ERROR());
                //exit(1);
                break;
              }

              conn_info *addr = (conn_info *)malloc(sizeof(conn_info));
              if ( !addr )
              {
                printf("Memory allocation error!\n");
                break;
              }
/*
              if ( !getpeername(newSock, (struct sockaddr *)&(addr -> addr), &namelen) )
              {
                int port = ntohs(addr->addr.sin_port);
                char *host = inet_ntoa(addr->addr.sin_addr);
                printf("Connect from %s:%i\n", host, port);
              }
*/       
              addr->sock = newSock;

#ifndef SON_WIN32
              pthread_attr_init(&pth_attr);
              pthread_attr_setstacksize(&pth_attr, SON_STACKSIZE);
              pthread_attr_setdetachstate(&pth_attr, 1);
 
              if (pthread_create(&thread, &pth_attr, handle_connection, addr) != 0)
#else
              if(_beginthreadex(0, 0, handle_connection, addr, 0, 0) == 0)
#endif
              {
                printf("Create thread failed with error: %i\n", GET_ERROR());
              }
#ifndef SON_WIN32
              pthread_attr_destroy(&pth_attr);
#endif
            }

            printf("Finished serving. Closing sockets and cleaning up...\n");
            shutdown(mainSock, SHUT_RDWR);
            char buf[16];
            while ( recv(mainSock, buf, 16, 0) > 0 );
          }
        }
      }
      
      CLOSE(mainSock);
    }
  }
}

HANDLER handle_connection(void *arg)
{
  conn_info *info = (conn_info *)arg;
  int socket = info->sock;
  // Code here
 
   // libxml related
   _initThreadLogging();
 
  OMS_PAGE *p = NULL;
  char *buf = 0;  // Sic!
  
  int i = 0;
  int j = 0;
  int k = 0;

  /*
   * И тут нас ждет жопа
   * Дело в том, что recv не станет ждать, пока к нам прилетят все данные.
   * Он просто возвращает управление, когда скопировал все пришедшие ему 
   * (а это могут быть не все данные).
   * Отсюда такие костыли.
   */

  int clen = 0;
  char *b = 0;

  do
  {
    i = recv(socket, (buf = realloc(buf, k+1024))+k, 1024, 0/* | MSG_DONTWAIT*/);

    k += i;

    if ((clen == 0) && (i > 0))
    {
      b = strstr(buf, "\r\n\r\n");  // Все ли заголовки пришли
      if (!b) continue;
      char *c = strstr(buf, "Content-Length:");
      while (!isdigit(*c)) c++;
      clen = atoi(c);
      b += 4; // Устанавливаем указатель на начало полезных данных
//      printf("clen is %d\n", clen);
      j = k - (b - buf);
    } else {
      j += i;
    }
  }
  while (j < clen);  // Пока не получили все
    
  //printf("1");
    
  for (i = 0; i < j; i++)
    if (b[i] == 0) b[i] = ' ';
  b[j] = 0;
    
//  printf("Req: \n%s\n", b);
    
  char *url = strstr(b, "u=/obml/") + 8;
  if (url[1] == '/') url += 2;
  
  char *ws = strchr(url, ' ');
  
  char *new_url = 0;
  
  // Добавим к URL GET-данные
  char *get = strstr(b, "j=");  // j=URL=1&...
  
  *ws = 0;
  
  if (get)
  {
    get += 2;
    char *action = get;
    get = strchr(get, '=');
    *get = 0;
    get += 2;

    //printf("Was: %s\n", action);
    action = url_decode(action);   
    //printf("Now: %s\n", action);
    
    new_url = malloc(strlen(action) + strlen(get) + strlen(url) + 1); // Чтоб наверняка
    
    if (action != 0)
    {
      //if (strchr(action, '?'))      // Уже есть какие-то параметры
      // Запрос перенаправляется на другой URL
      if (strstr(action, "://"))  // Полный URL
        strcpy(new_url, action);
      else {   // URL неполный
      
        char *sl = 0;
        
        if (action[0] == '/')  // Абсолютный путь
        {
          sl = strchr(url + 7, '/');
          action++;
        } else { // Относительный
          sl = strrchr(url, '/');
        }
        
        if (sl)
          *sl = 0;
          
        sprintf(new_url, "%s/%s", url, action);
      }
    } else {
      strcat(new_url, url);
    }
    
    if (strchr(new_url, '?'))   // Уже есть параметры
      *get = '&';
    else
      *get = '?';
    strcat(new_url, get);
  } else {
    new_url = strdup(url);
  }
  
  memset(buf, 0, j);
  free(buf);
  buf = NULL;
    
  p = oms_load_page(new_url);
  
  free(new_url);

  //printf("2");

  if (p == NULL)
  {
    printf("Some occasion!\n");
  } else {
    int tsize = p->size + sizeof(OMS_HEADER_COMMON);
//    printf("size is %d\n", tsize);
    char *answer_t = "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nConnection: close\r\nContent-Length: %d\r\n\r\n";
    char *answer = malloc(strlen(answer_t) + 11);
    sprintf(answer, answer_t, tsize);

    i = send(socket, answer, strlen(answer), 0);
    j = send(socket, p->data, tsize, 0);

//    printf("Res: %d, %d\n", i, j);
  }

  oms_free_page(p);

  // Cleanup 
  shutdown(socket, SHUT_RDWR);
  
  CLOSE(socket);

  free(arg);
  printf("Done\n");
  pthread_exit(0);
}

static unsigned char ch(unsigned char c)
{
  if((c >= '0') && (c <= '9'))
    return c - '0';
    
  if((c >= 'A') && (c <= 'Z'))
    return c - 'A' + 10;
    
  if((c >= 'a') && (c <= 'z'))
    return c - 'a' + 10;

  return 0;
}

/*
 * Функция преобразует URL из такого вида:
 * http%3a%2f%2fyandex.ru%2fyandsearch
 * в такое:
 * http://yandex.ru/yandsearch
 * результат записывается поверх в ту же самую строку
 */
 
char *url_decode(char *url)
{
  char *ret = url;
  char *new = url;

  while (*url)
  {
    if (*url == '%')
    {
      *url++;
      *new = (ch(*url)<<4)|ch(*(++url));
    } else {
      *new = *url;
    }
    
    new++;
    url++;
  } 
  *new = 0;  
  return ret;
}
