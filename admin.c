#include "includes.h"

int main(int argc, char *argv[])
{

  printf("--> ADMIN CLIENT <--\n");

  /*We can proceed to make connection with our server. */

  /* "sockaddr_in" struct to store the server's address. */
  struct sockaddr_un server_addr;
  memset(&server_addr, 0, sizeof(struct sockaddr_un));
  server_addr.sun_family = AF_UNIX;
  strncpy(server_addr.sun_path, UNIXSOCKET, sizeof(server_addr.sun_path) - 1);

  /*We create the TCP socket for communicating with the server. */

  int sock = socket(AF_UNIX, SOCK_STREAM, 0);

  if (sock == -1)
  {
    printf("\nERROR! Socket creation failed\n ");
    return 2;
  }

  /*Once we have our socket we can connect() to our server*/

  if (connect(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_un)) == -1)
  {
    /*If it returned -1, then an error occured.
    We inform the user and quit.
    */
    printf("\nERROR! Cannot connect to server !\n  Please try again later! \n");
    return 3;
  }

  /*Once connection is successfully established we can send and receive data between our client and server application. */

  /*Buffer that can be used for sending and receiving text responses between server and administrator client. */
  char response[1024];
  memset(response, 0x00, 1024);

    /*We read the response from the server. If response is "WELCOME", then we are good to go. */

  if (read(sock, response, 1024) < 0)
  {
    perror("ERROR! ---> read() ");
    printf("\nERROR! ---> read()\n");
    return 5;
  }

  if (strcmp(response, "WELCOME") != 0)
  {
    /*If response is not the string "WELCOME", the server rejected our connection. */
    printf("\nERROR! CONNECTION REJECTED!\n");

    close(sock);
    return 6;
  }

  /*Print success message to let the user know that the server accepted the connection. */
  printf("\n!!! CONNECTION ACCEPTED !!! \n\n");

  /*We constantly display a menu before the user...
   * So that they can use the options to carry out their administrator capabilities.
   */

  int op = 0; /*int that stores user supplied option. */

  while (1)
  {
    printf("\n----- MENU -----\n");
    printf("  1. View Number OF Clients Connected. \n");
    printf("  2. View Server Statistics. \n");
    printf("  3. Stop Server. \n");
    printf("  4. End Session. \n");
    printf("> ");
    /*Read the user supplied option */
    scanf("%d", &op);

    if ((op < 1) || (op > 3))
    {
      printf("\nERROR! Invalid option!\n");
      continue;
    }

    if (op == 1)
    {
      /*We inform the server that we need the servers statistical information */

      if (write(sock, "STATS", strlen("STATS") + 1) < 0)
      {
        perror("ERROR! ---> write() ");
        continue;
      }

      /*We reset the response buffer so that we can store the report send by the server */
      if (read(sock, response, 1024) < 0)
      {
        perror("ERROR! ---> read() ");
        continue;
      }

      /*Finally we display the statistical information captured to the terminal screen of administrator client. */

      printf("\n----- CONNECTED CLIENTS -----\n%s\n", response);
    }
    else if (op == 2)
    {
      /*We send the server the message "SHUTDOWN" to initiate the server shutdown process. */

      printf("\nI: Shutting down server... \n");

      if (write(sock, "SHUTDOWN", strlen("SHUTDOWN") + 1) < 0)
      {
        perror("ERROR! ---> write() ");
        continue;
      }

      /*We prepare to read the response from tbe server. */
      memset(response, 0x00, 1024);

      if (strcmp(response, "BYE") == 0)
      {
        printf("\n---> Shutdown process initiated within server... \n");
      }

      break;
    }
    else if (op == 3)
    {
      /*We tell the server that we are ending our session and our connection should be closed. */

      printf("\n---> Ending session... \n");

      if (write(sock, "EXIT", strlen("EXIT") + 1) < 0)
      {
        perror("ERROR! ---> write() ");
        continue;
      }

      break;
    }
  }

  printf("\n---> ADMIN SESSION ENDED <---\n");

  /*Close the socket after use. */
  close(sock);

  return 0;
}
