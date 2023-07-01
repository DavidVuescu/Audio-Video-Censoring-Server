#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <pthread.h>

#define UNIXSOCKET "/tmp/unix_socket"
#define INETPORT 18081

typedef struct CLIENT_DATA
{
  int client_socket_fd;
  struct sockaddr_in client_addr;

} CLIENT_DATA;

typedef struct ADMIN_DATA
{
  int admin_socket_fd;
  struct sockaddr_un admin_addr;

} ADMIN_DATA;

typedef struct SERVER_INFO
{
  /*Number of clients connected */
  uint32_t num_clients;
  int serv_sock_fd;

} SERVER_INFO;


pthread_mutex_t pmutex;
/* Global struct instance of SERVER_STAT*/
SERVER_INFO server_info;
/*Global flag that controls the service loop of the server. */
int run = 1;

void *unix_main(void *args);
void *inet_main(void *args);
int censor_video( char *videoPath,  char *output_path);
void *admin_handler(void *data);
void *client_handler(void *data);
void *connection_handler(void *data);
void* executePythonScript(void* arg);
void* censor_video_thread(void* arg);

