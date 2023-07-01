#include "includes.h"

int main(int argc, char *argv[])
{

  printf("--> USER CLIENT <--\n");

  /*If the user has not provided the right number of arguments. Print usage instructions and return */
  if (argc != 3)
  {
    printf("\nUsage:\n%s <Server IPv4 address> <port> \n", argv[0]);
    return 1;
  }

  /*We can proceed to make connection with our server. */

  /* "sockaddr_in" struct to store the server's address. */
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr(argv[1]);
  server_addr.sin_port = htons(atoi(argv[2]));

  /*We create the TCP socket for communicating with the server. */

  int sock = socket(AF_INET, SOCK_STREAM, 0);

  if (sock == -1)
  {
    printf("\nERROR! --->  Socket creation failed\n");
    return 2;
  }

  /*Once we have our socket we can connect() to our server*/

  if (connect(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in)) == -1)
  {
    /*If it returned -1, then an error occured.
    We inform the user and quit.
    */
    printf("\nERROR! Cannot connect to server !\n   Please try again later! \n");
    return 3;
  }

  /*Once connection is successfully established we can send and receive data between our client and server application. */

  /*Buffer that can be used for sending and receiving text responses. */
  char response[1024];
  memset(response, 0x00, 1024);

  /*Dynamically allocated buffer for receiving / sending bytes*/

  unsigned char *buff = (unsigned char *)malloc(4096);

  
  /*We read the response from the server. If response is "WELCOME", then we are good to go. */

  if (read(sock, response, 1024) < 0)
  {
    printf("\nERROR! ---> read()\n");
    return 5;
  }

  if (strcmp(response, "WELCOME") != 0)
  {
    /*If response is not the string "WELCOME", the server rejected our connection. */
    printf("\nERROR! ---> CONNECTION REJECTED!\n");
    free(buff);
    close(sock);
    return 6;
  }

  printf("\n!!! CONNECTION ACCEPTED !!! \n\n");
  /*Now we present the user with a menu to either access the service provided by the server or exit the session. */

  int op = 0; /*int that stores user supplied option. */

  while (1)
  {
    printf("\n---- MENU ----\n");
    printf("  1. Upload video to server for censoring\n");
    printf("  2. Exit session. \n");
    printf("> ");

    /*Read the user supplied option */
    scanf("%d", &op);

    /*Validate the option. */

    if ((op < 1) || (op > 2))
    {
      printf("\nE: Please provide correct option from the list and try again!\n");
      continue;
    }

    if (op == 1)
    {
      char *path = malloc(128);
      memset(path, 0x00, 128);

      printf("\nPlease type path to video file: ");

      char ch;
      int idx = 0;
      ch = fgetc(stdin);
      ch = 0;
      while ((ch = fgetc(stdin)) != '\n')
      {
        path[idx] = ch;
        idx++;
      }

      path[idx] = 0;
      /*We attempt to stat the file. If its not found stat() will return -1*/

      struct stat file_stat;
      memset(&file_stat, 0x00, sizeof(struct stat));
      if (stat(path, &file_stat) == -1)
      {
        /*If stat failed we report that to the user and continue the loop. */
        perror("stat() ");
        continue;
      }

      printf("\nI: Preparing to upload file: '%s' of size: %jd bytes. \n", path, (intmax_t)file_stat.st_size);

      /*Otherwise we send a valid response to our server. */
      // Reset the response buffer.
      memset(response, 0x00, 1024);

      /*First we need to find the extension of the image file. It can be done using strrchr() */

      // We prepare the response buffer.

      sprintf(response, "%s %jd", strrchr(path, '.'), (intmax_t)file_stat.st_size);

      /*We send the response to the server. */

      if (write(sock, response, strlen(response) + 1) < 0)
      {
        perror("write() ");
      }

      /*Reset the response buffer*/
      memset(response, 0x00, 1024);
      /*We read the response from the server */

      if (read(sock, response, 1024) < 0)
      {
        perror("read() ");
      }

      /*If response is the string "SEND_FILE" then all is okay and we can proceed to upload our image file. */
      if (strcmp(response, "SEND_FILE") != 0)
      {
        /*Unexpected error... */
        printf("\nE: Unexpected change in communication protocols.\n   Terminating... \n");
        break;
      }

      /*We upload our image file to our server.  */

      int n = 0; // Number of bytes instantaneously read by fread()

      FILE *fp = fopen(path, "rb"); // Open the image file...

      while ((n = fread(buff, 1, 4096, fp)) > 0)
      {
        /*We send the read data to the server... */

        if (write(sock, buff, n) < 0)
        {
          perror("write() ");
          break;
        }
      }

      /*We properly close the opened file. */

      fclose(fp);

      

      /*We ask for the user to wait for a while as this process is time consuming... */
      printf("\nI: Processing... \n   Waiting for response from the server... \n");

      memset(response, 0x00, 1024);
      /*We read the response from the server. */

      if (read(sock, response, 1024) < 0)
      {
        perror("read() ");
        break;
      }

      /*If response contains the word SUCCESS then the process is successfull */

      if (strstr(response, "SUCCESS") != NULL)
      {
        /*Now we parse out the size of the resulting video from the server response which is always in format "SUCCESS VIDEO_SIZE" */

        printf("\nI: The server successfully created the video file! \n");

        intmax_t video_size = atoll(strstr(response, " ") + 1);

        /*Now we tell the server to start sending us the file so that we can download it to our local storage... \n*/

        if (write(sock, "SEND_VIDEO", strlen("SEND_VIDEO") + 1) < 0)
        {
          perror("write() ");
          break;
        }

        /*In the meantime we ask the user to provide a file with same extension to save the downloaded file to local storage... */

        do
        {
          printf("\nPlease provide path to download video (Please use same extension provided earlier...): ");

          /*We read the new path from the user. */
          ch = fgetc(stdin);
          ch = 0;
          idx = 0;
          while ((ch = fgetc(stdin)) != '\n')
          {
            path[idx] = ch;
            idx++;
          }

          path[idx] = 0;
        } while ((fp = fopen(path, "wb")) == NULL);

        /*Number of bytes instantaneously read from the socket*/
        int num_bytes = 0;
        /*Total bytes received. */
        intmax_t sz = 0;

        while (sz < video_size)
        {
          num_bytes = read(sock, buff, 4096);

          if (num_bytes == -1)
          {
            /*In the event of an unexpected error */

            perror("read() ");
            break;
          }

          /*We write the data to the video file. */

          fwrite(buff, 1, num_bytes, fp);
          /*Update the total number of bytes read */
          sz += num_bytes;
        }

        /*Finally we close the file after writing. */

        fclose(fp);

        /*Inform the user that the operation has been successfully completed. */

        printf("\nI: Success! The video file has been downloaded to: '%s' \n", path);

        free(path); // Free the allocated buffer for storing path input from user.
      }
      else
      {
        /*If there is an error we report it to the user and continue the looping, so that user can try again later. */
        printf("\nE: A server internal error occured while processing the video :( \n   Please try again later, with different parameters...\n");
        continue;
      }
    }
    else if (op == 2)
    {
      printf("\nI: Ending session... \n");
      /*We send the exit message to our server */
      write(sock, "EXIT", strlen("EXIT") + 1);

      break;
    }
  }

  printf("\nThankyou very much, for using the user client application ! \n");

  /*We free dynamically allocated buffer. */
  free(buff);

  /*We close the socket... */
  close(sock);

  return 0;
}
