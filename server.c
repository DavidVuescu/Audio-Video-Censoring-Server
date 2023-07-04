#include "includes.h"

int main()
{
  int iport;

  pthread_t unixthr, /* UNIX Thread: the UNIX server component */
      inetthr;       /* INET Thread: the INET server component */
       
  unlink(UNIXSOCKET);
  printf("\n............ SERVER ............ \n");

  pthread_create(&unixthr, NULL, unix_main, UNIXSOCKET);
  iport = INETPORT;
  pthread_create(&inetthr, NULL, inet_main, &iport);
  /*
    pthread_create (&workerthr, NULL, work_main, NULL) ;
   */
  pthread_join(unixthr, NULL);
  pthread_join(inetthr, NULL);

  unlink(UNIXSOCKET);
  return 0;
}


void *unix_main(void *args)
{
  memset(&server_info, 0x00, sizeof(SERVER_INFO));
  /* sockaddr_in struct to store,
     the server network info like IP, port.
  */
  struct sockaddr_un serv_addr;
  /*Zero the memory out. */
  memset(&serv_addr, 0x00, sizeof(struct sockaddr_un));

  serv_addr.sun_family = AF_UNIX;
  strncpy(serv_addr.sun_path, UNIXSOCKET, sizeof(serv_addr.sun_path) - 1);
  // serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  // /*Set the port. */
  // serv_addr.sin_port = htons(INETPORT);

  /*First we create a TCP Stream socket. */
  int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);

  if (sock_fd == -1)
  {
    printf("\nERROR! ---> Socket creation failed.\n");
    pthread_exit(NULL);
  }

  /*We copy our socket file descriptor to the corresponding variable in the global server stats struct instance */
  server_info.serv_sock_fd = sock_fd;

  /*Now we bind the socket with our server address*/
  if (bind(sock_fd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr_un)) == -1)
  {
    printf("\n ERROR! ---> Failed to bind socket\n");
    pthread_exit(NULL);
  }

  /*Now we make our socket ready to receive incomming connections. */
  if (listen(sock_fd, 5) == -1)
  {
    printf("\nERROR! ---> Failed to listen for incoming connections\n");
  }

  /*Before we set up the service loop ; we initialize the mutex.
     The mutex, ensures thread safety.
  */

  pthread_mutex_init(&pmutex, NULL);
  /*Array to store the thread IDs for the threads that will be created. */
  /*We initially allocate for one thread id, then we aloocate more if required*/
  pthread_t *thread_ids = (pthread_t *)malloc(sizeof(pthread_t));
  uint32_t n = 1; // Variable that keeps track of the number of thread ids allocated so far.
  /*Index into the thread ID array*/
  uint32_t idx = 0;

  /*Now we start the infinite service loop of the server application.
   * So long as the 'run' flag is set to one, the loop runs.
   */

  printf("\n---> SERVER LISTENING FOR UNIX CONNECTIONS...\n");

  while (1)
  {
    /*We need to check the global run variable. */
    int value_run = 0;
    /*We copy the value safely...
      We lock before accessing the critical section and unlock it after access.
    */
    pthread_mutex_lock(&pmutex);
    value_run = run;
    pthread_mutex_unlock(&pmutex);
    if (value_run == 0)
    {
      /*This means we need to stop our service loop, and not accept any further connections... */
      break;
    }

    /*To store the client's socket file descriptor. */
    int cl_sock = 0;

    /* sockaddr_in struct to store the address of client application. */
    struct sockaddr_un cli_addr;
    socklen_t cli_addr_size = sizeof(struct sockaddr_un);
    cl_sock = accept(sock_fd, (struct sockaddr *)&cli_addr, &cli_addr_size);

    // Verificam daca avem un admin conectat
    pthread_mutex_lock(&pmutex);
    if (admin_connected)
    {
      pthread_mutex_unlock(&pmutex);

      // Refuzam conexiunea
      close(cl_sock);
      continue;
    }
    pthread_mutex_unlock(&pmutex);

    if (cl_sock == -1)
    {
      /*An error occured in accepting this connection... We skip it! */
      pthread_mutex_lock(&pmutex);
      printf("\nERROR! ---> Failed to accept connection.\n");
      pthread_mutex_unlock(&pmutex);
      continue;
    }
    else{
      // Setam flag-ul de admin_connected 
      pthread_mutex_lock(&pmutex);
      admin_connected = 1;
      pthread_mutex_unlock(&pmutex);
    }

    /*We store the information about our new connection in a CLIENT_DATA struct that is heap allocated and passed on as argument to the thread function. */
    ADMIN_DATA *data = (ADMIN_DATA *)malloc(sizeof(ADMIN_DATA));

    /*Initialize it with required values. */
    data->admin_socket_fd = cl_sock;
    memcpy(&data->admin_addr, &cli_addr, sizeof(struct sockaddr_un));

    if (idx == n)
    {
      /*We need to allocate more thread IDs*/
      thread_ids = realloc(thread_ids, (n + 1) * sizeof(pthread_t));

      n += 1; // Update the allocation count.
    }

    /*We create a thread to handle this present connection. */
    if (pthread_create(&thread_ids[idx], NULL, admin_handler, (void *)data) == -1)
    {
      /*If there is an error we report it to the user and skip handling of this connection */

      pthread_mutex_lock(&pmutex);
      printf("\nERROR! ---> Thread not created.\n ");
      pthread_mutex_unlock(&pmutex);

      continue;
    }

    /*We have successfully spawned the thread, now we increment the index */
    idx++;
  }

  /*We join all our threads created so far to our main thread*/

  for (int i = 0; i < n; i++)
  {
    pthread_join(thread_ids[i], NULL);
  }

  /*We can safely destroy the mutex, because it has served its purpose */
  pthread_mutex_destroy(&pmutex);

  /*Free the thread ID array. */
  free(thread_ids);

  printf("\n---> SERVER SHUTDOWN <---\n");
}

void *inet_main(void *args)
{

  /* sockaddr_in struct to store,
     the server network info like IP, port.
  */
  struct sockaddr_in serv_addr;
  /*Empty the memory out. */
  memset(&serv_addr, 0x00, sizeof(serv_addr));

  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  /*Set the port. */
  serv_addr.sin_port = htons(INETPORT);

  /*Create a TCP Stream socket. */
  int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

  if (sock_fd == -1)
  {
    printf("\nERROR! ---> Socket creation failed.\n");
  }

  /*We copy our socket file descriptor to the corresponding variable in the global server stats struct instance */
  server_info.serv_sock_fd = sock_fd;

  /*Now we bind the socket with our server address*/
  if (bind(sock_fd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr_in)) == -1)
  {
    printf("\n ERROR! ---> Failed to bind socket\n");
  }

  /*Now we make our socket ready to receive incomming connections. */
  if (listen(sock_fd, NR_CONNECTIONS) == -1)
  {
    printf("\nERROR! ---> Failed to listen for incoming connections\n");
  }

  /*Before we start any service we need to make sure two important directories are existent in our current working directory of our server.

  We check whether "ServerInput" and "ServerOutput" directory is present in the current working directory.

  If they are present we dont bother; else we create them because they are necessary for our server's proper functioning.
  */

  struct stat check_dir;

  printf("\n");

  if (stat("./ServerInput", &check_dir) == -1)
  {
    /*If stat failed then we need to create the directory with read write execute for owner group & others*/

    if (mkdir("./ServerInput", 0777) != 0)
    {
      /*If mkdir failed we terminate the server with error message */
      printf("ERROR! ---> Directory:'./ServerInput' was not created\n");
    }
    else
    {
      printf("SUCCESS! ---> Directory:'./ServerInput' created\n");
    }
  }

  memset(&check_dir, 0x00, sizeof(struct stat));

  /*We do the same for second directory. */

  if (stat("./ServerOutput", &check_dir) == -1)
  {
    /*If stat failed then we need to create the directory with read write execute for owner group & others*/

    if (mkdir("./ServerOutput", 0777) != 0)
    {
      /*If mkdir failed we terminate the server with error message */
      printf("ERROR! ---> Directory:'./ServerOutput'\n was not created\n");
    }
    else
    {
      printf("SUCCESS! ---> Directory:'./ServerOutput' created\n");
    }
  }

  /*Before we set up the service loop ; we initialize the mutex.
     The mutex, ensures thread safety.
  */

  pthread_mutex_init(&pmutex, NULL);
  /*Array to store the thread IDs for the threads that will be created. */
  /*We initially allocate for one thread id, then we aloocate more if required*/
  pthread_t *thread_ids = (pthread_t *)malloc(sizeof(pthread_t));
  uint32_t n = 1; // Variable that keeps track of the number of thread ids allocated so far.
  /*Index into the thread ID array*/
  uint32_t idx = 0;

  printf("\n---> SERVER LISTENING FOR INET CONNECTIONS...\n");

  while (1)
  {
    /*We need to check the global run variable. */
    int value_run = 0;
    /*We copy the value safely...
      We lock before accessing the critical section and unlock it after access.
    */
    pthread_mutex_lock(&pmutex);
    value_run = run;
    pthread_mutex_unlock(&pmutex);
    if (value_run == 0)
    {
      /*This means we need to stop our service loop, and not accept any further connections... */
      break;
    }

    /*To store the client's socket file descriptor. */
    int cl_sock = 0;

    /* sockaddr_in struct to store the address of client application. */
    struct sockaddr_in cli_addr;
    socklen_t cli_addr_size = sizeof(struct sockaddr_in);
    cl_sock = accept(sock_fd, (struct sockaddr *)&cli_addr, &cli_addr_size);

    if (cl_sock == -1)
    {
      /*An error occured in accepting this connection... We skip it! */
      pthread_mutex_lock(&pmutex);

      printf("\nERROR! ---> Failed to accept connection.\n");
      pthread_mutex_unlock(&pmutex);
      continue;
    }

    /*We store the information about our new connection in a CLIENT_DATA struct that is heap allocated and passed on as argument to the thread function. */
    CLIENT_DATA *data = (CLIENT_DATA *)malloc(sizeof(CLIENT_DATA));

    /*Initialize it with required values. */
    data->client_socket_fd = cl_sock;
    memcpy(&data->client_addr, &cli_addr, sizeof(struct sockaddr_in));

    if (idx == n)
    {
      /*We need to allocate more thread IDs*/
      thread_ids = realloc(thread_ids, (n + 1) * sizeof(pthread_t));

      n += 1; // Update the allocation count.
    }

    /*We create a thread to handle this present connection. */
    if (pthread_create(&thread_ids[idx], NULL, client_handler, (void *)data) == -1)
    {
      /*If there is an error we report it to the user and skip handling of this connection */

      pthread_mutex_lock(&pmutex);
      printf("\nERROR! ---> Thread not created.\n ");
      pthread_mutex_unlock(&pmutex);

      continue;
    }

    /*We have successfully spawned the thread, now we increment the index */
    idx++;
  }

  /*We join all our threads created so far to our main thread*/

  for (int i = 0; i < n; i++)
  {
    pthread_join(thread_ids[i], NULL);
  }

  /*We can safely destroy the mutex, because it has served its purpose */
  pthread_mutex_destroy(&pmutex);

  /*Free the thread ID array. */
  free(thread_ids);

  printf("\n---> SERVER SHUTDOWN <---\n");
}

/*Function that handles an administrator client. */
void *admin_handler(void *data)
{

  ADMIN_DATA *cdata = (ADMIN_DATA *)data;
  pthread_mutex_lock(&pmutex);
  printf("\n---> ADMIN CLIENT connected <---\n");

  /*Update the number of clients. */
  server_info.num_clients++;

  /*Release the lock*/
  pthread_mutex_unlock(&pmutex);
  /*We create a copy of the client socket file descriptor. */
  int cl_sock = cdata->admin_socket_fd;

  /*The server sends a WELCOME message to client to let them know they are accepted. */
  if (write(cl_sock, "WELCOME", strlen("WELCOME") + 1) < 0)
  {
    /*If error occurs, we close the connection and return
     * We also decrement the appropriate counts from the statistics*
     */
    pthread_mutex_lock(&pmutex);
    server_info.num_clients--;
    pthread_mutex_unlock(&pmutex);
    close(cl_sock);
  }

  /*Buffer to store a response strings... */
  char response[1024];

  /*We enter into our loop of service for the administrator client.
   * We process their valid requests and sends them the required responses */

  while (1)
  {
    memset(response, 0x00, 1024);
    /*Read the response from user client */
    if (read(cl_sock, response, 1024) < 0)
    {
      break;
    }

    /*If the response is the string "EXIT" we end this session with the administrator client. */
    if (strcmp(response, "EXIT") == 0)
    {
      pthread_mutex_lock(&pmutex);
      /*We print on the terminal that the session for this administrator client is over. */
      printf("\n---> ADMIN CLIENT left!\n");
      pthread_mutex_unlock(&pmutex);
      break;
    }
    else if (strcmp(response, "CLIENTS") == 0)
    {
      /*The administrator client wants the servers statistics. */
      /*We prepare the response buffer and send it to the admin client that contain the servers present statistics. */
      memset(response, 0x00, 1024);

      pthread_mutex_lock(&pmutex);

      sprintf(response, "Number of clients connected: %d\n", server_info.num_clients);
      pthread_mutex_unlock(&pmutex);

      /*We send the response to the admin client. */
      if (write(cl_sock, response, strlen(response) + 1) < 0)
      {
        break;
      }

      pthread_mutex_lock(&pmutex);

      printf("\n---> Number of clients connected infomation sent to admin \n");
      pthread_mutex_unlock(&pmutex);
    }
    else if (strcmp(response, "STATS") == 0)
    {
      /*The administrator client wants the servers statistics. */
      /*We prepare the response buffer and send it to the admin client that contain the servers present statistics. */
      memset(response, 0x00, 1024);

      pthread_mutex_lock(&pmutex);

      sprintf(response, "Number of videos processed: %d\n", server_info.successful_services);
      pthread_mutex_unlock(&pmutex);

      /*We send the response to the admin client. */
      if (write(cl_sock, response, strlen(response) + 1) < 0)
      {
        break;
      }

      pthread_mutex_lock(&pmutex);

      printf("\n---> Server infomation sent to admin \n");
      pthread_mutex_unlock(&pmutex);
    }
    else if (strcmp(response, "SHUTDOWN") == 0)
    {
      /*The administrator client wants the server application to stop ! */

      /*We send a BYE message to the client to indicate shutdown process has been initiated. */
      write(cl_sock, "BYE", strlen("BYE") + 1);

      /*The global run variable is set to zero to stop the server from accepting further connections. */
      pthread_mutex_lock(&pmutex);
      run = 0;

      printf("\nI: ---> SERVER SHUTDOWN <--\n INITIATOR: Admin client\n");
      pthread_mutex_unlock(&pmutex);

      /*Finally the server's socket is shut downed forcibily. */

      shutdown(server_info.serv_sock_fd, SHUT_RD);
      break;
    }
  }
  //Change the admin_connection flag
  pthread_mutex_lock(&pmutex);
  admin_connected = 0;
  pthread_mutex_unlock(&pmutex);

  // /*Since we are closing the connection we decrement the counts*/
  pthread_mutex_lock(&pmutex);
  server_info.num_clients--;
  pthread_mutex_unlock(&pmutex);

  /*We close the connection after service. */
  close(cl_sock);
}

/*Function that handles a user client. */
void *client_handler(void *data)
{
  CLIENT_DATA *cdata = (CLIENT_DATA *)data;
  pthread_mutex_lock(&pmutex);
  printf("\n---> USER CLIENT connected ( IP: %s,  PORT: %d)\n", inet_ntoa(cdata->client_addr.sin_addr), ntohs(cdata->client_addr.sin_port));

  /*Update the number of clients*/
  server_info.num_clients++;

  /*Release the lock*/
  pthread_mutex_unlock(&pmutex);
  /*We create a copy of the client socket file descriptor. */
  int cl_sock = cdata->client_socket_fd;

  /*The server sends a WELCOME message to client to let them know they are accepted. */
  if (write(cl_sock, "WELCOME", strlen("WELCOME") + 1) < 0)
  {
    /*If error occurs, we close the connection and return
     * We also decrement the appropriate counts from the statistics*
     */
    pthread_mutex_lock(&pmutex);
    server_info.num_clients--;
    pthread_mutex_unlock(&pmutex);
    close(cl_sock);
  }

  /*Buffer to store a response strings... */
  char response[64];
  
  char option[64];

  /*The server needs to store video uploaded by user in its local storage. */
  char video_path[128];

  /*Path of the resulting video in the local storage of server*/
  char output_video_path[128];

  /*We need buffers to store video file extension from the user client. */
  char video_file_ext[31];

  /*Required video file extension. */
  char file_ext[8];

  /*We allocate a buffer to make the task of reading and writing files more manageable. */
  unsigned char *buff = (unsigned char *)malloc(4096);

  intmax_t fSize = 0;


  /*We seed the random number generator. */
  srandom(time(NULL));

  /*We enter into a while loop of service in which we accept video files from the user client with aporopriate parameters and does the expected job for them. */
  while (1)
  {
    /*Reset the all the buffers as well as other variables. */
    memset(response, 0x00, 64);

    memset(video_path, 0x00, 128);
    memset(video_file_ext, 0x00, 31);

    memset(output_video_path, 0x00, 128);
    memset(file_ext, 0x00, 8);

    fSize = 0;


    /*Read the response from user client */
    if (read(cl_sock, response, 64) < 0)
    {
      break;
    }


    /*If the response is the string "EXIT" we quit serving the client immediately. */
    if (strcmp(response, "EXIT") == 0)
    {
      pthread_mutex_lock(&pmutex);
      /*We print on the terminal that the session for this user client is over. */
      printf("\n---> USER CLIENT left!  (IP: %s on port: %d)\n", inet_ntoa(cdata->client_addr.sin_addr), ntohs(cdata->client_addr.sin_port));
      pthread_mutex_unlock(&pmutex);
      break;
    }
	
	
	strcpy(option, response);
    
    //printf("%d----\n", *option);
	
	if (read(cl_sock, response, 64) < 0)
    {
      break;
    }
	
	//printf("%s\n", response);
	
    
    /*We extract the video file extension and size from the response string. */
    sscanf(response, "%s %jd", video_file_ext, (intmax_t *)&fSize);

    /*If the file size is zero or video file extension is empty. We close this connection */

    if ((fSize == 0) || strlen(video_file_ext) == 0)
    {
      pthread_mutex_lock(&pmutex);
      printf("I: User client (IP = %s, port = %d), \n   sent invalid data; Closing connection!\n", inet_ntoa(cdata->client_addr.sin_addr), ntohs(cdata->client_addr.sin_port));
      pthread_mutex_unlock(&pmutex);

      break;
    }

    /*We print necessary information on the server's terminal */

    pthread_mutex_lock(&pmutex);
    printf("I: User client (IP = %s, port = %d), \n   wants to upload a '%s' video file, having size %jd bytes. \n", inet_ntoa(cdata->client_addr.sin_addr), ntohs(cdata->client_addr.sin_port), video_file_ext, fSize);

    pthread_mutex_unlock(&pmutex);

    /*The server then sends a "SEND_FILE" response to indicate that the client should begin the upload of the file to the server. */

    if (write(cl_sock, "SEND_FILE", strlen("SEND_FILE") + 1) < 0)
    {
      break;
    }

    /*We use random numbers to create a unique file name for the uploaded video, so that it won't create a problem if a different thread gets same file name from user */
    sprintf(video_path, "./ServerInput/%ld_video_%s", random(), video_file_ext);

    /*Now we read the file contents from the client and save it in our server's local storage. */

    intmax_t sz = 0; // Size counter.
    /*We prepare to save the data read from the client ... */
    FILE *fp = fopen(video_path, "wb");
    int n = 0; // Number of bytes instantaneously read.

    while (sz < fSize)
    {
      n = read(cl_sock, buff, 4096);
      if (n == -1)
      {
        /*Error occured! */
        pthread_mutex_lock(&pmutex);
        server_info.num_clients--;
        pthread_mutex_unlock(&pmutex);
        /*Close and free resources before return. */
        close(cl_sock);
        fclose(fp);
        free(buff);
      }
      /*We write the data read to our file. */
      fwrite(buff, 1, n, fp);
      sz += n; /*Increment the size wrote so far... */
    }

    /*Once thats done we close the file. */
    fclose(fp);

    pthread_mutex_lock(&pmutex);
    printf("\nI: Successfully received a '%s' video file from user client (IP = %s, port = %d) \n", video_file_ext, inet_ntoa(cdata->client_addr.sin_addr), ntohs(cdata->client_addr.sin_port));
    pthread_mutex_unlock(&pmutex);

   
   

    /*The response buffer is reset. */
    memset(response, 0x00, 64);
    /*Now we read the response */

   

    sprintf(output_video_path, "./ServerOutput/%ld_video_.%s", random(), video_file_ext);

    if (censor_video(option, video_path, output_video_path) == 0)
    {
      pthread_mutex_lock(&pmutex);
      printf("\nI: Successfully created video using FFMPEG, for user client (IP = %s, port = %d)\n", inet_ntoa(cdata->client_addr.sin_addr), ntohs(cdata->client_addr.sin_port));
      pthread_mutex_unlock(&pmutex);

      /*We need to obtain the file size of resulting video file*/
      struct stat file_stat;
      stat(output_video_path, &file_stat);

      /*We send a SUCCESS response, indicating the video conversion got successfully done! We also send the video file size along with it in "SUCCESS SIZE" format */
      memset(response, 0x00, 64);
      sprintf(response, "SUCCESS %jd", (intmax_t)file_stat.st_size);
      if (write(cl_sock, response, strlen(response) + 1) < 0)
      {
        break;
      }

      /*Now we read the response from client. */
      memset(response, 0x00, 64);
      if (read(cl_sock, response, 64) < 0)
      {
        break;
      }

      /*If their response is SEND_VIDEO we start sending the video file to the client. */
      if (strcmp(response, "SEND_VIDEO") == 0)
      {
        /*We send the video file to the user client*/

        fp = fopen(output_video_path, "rb");

        while ((n = fread(buff, 1, 4096, fp)) > 0)
        {
          if (write(cl_sock, buff, n) < 0)
          {
            pthread_mutex_lock(&pmutex);
            server_info.num_clients--;
            pthread_mutex_unlock(&pmutex);
            close(cl_sock);
            fclose(fp);
            free(buff);
            close(cl_sock);
          }
        }

        fclose(fp); // We close the file after reading..

        pthread_mutex_lock(&pmutex);
        printf("\nI: Successfully sent video of size: %jd bytes, to user client (IP = %s, port = %d)\n", (intmax_t)file_stat.st_size, inet_ntoa(cdata->client_addr.sin_addr), ntohs(cdata->client_addr.sin_port));
        pthread_mutex_unlock(&pmutex);

        /*With that we have successfully converted an image to a video with transition desired by user.
         * Therefore we increment the success counter */
        pthread_mutex_lock(&pmutex);
        server_info.successful_services++;
        pthread_mutex_unlock(&pmutex);
      }
    }
    else
    {
      /*Some error occured :( */ /*We send an ERROR response to the client. They should try again. */

      pthread_mutex_lock(&pmutex);

      printf("\nE: A fatal ERROR occured in creating the video with FFMPEG for user client (IP = %s, port = %d). \n", inet_ntoa(cdata->client_addr.sin_addr), ntohs(cdata->client_addr.sin_port));
      pthread_mutex_unlock(&pmutex);

      if (write(cl_sock, "ERROR", strlen("ERROR") + 1) < 0)
      {
        break;
      }
    }
  }

  /*We decrement the appropriate counts as well as increment the successful connections counter. */
  pthread_mutex_lock(&pmutex);
  server_info.num_clients--;
  pthread_mutex_unlock(&pmutex);

  /*We close the connection after service. */
  close(cl_sock);

  /*Free the buffer.*/
  free(buff);
}


int censor_video(char *option, char *videoPath, char *outputPath) {
    pid_t pid;
    pid = fork();   // Fork the process
    
    
	char option_char[32]; // Make sure the buffer is large enough to hold the string representation
    sprintf( option_char, "%d", *option);


    if (pid < 0) {
        printf("Failed to create child process.\n");
        return 1;
    } else if (pid == 0) {
        /* Child process */
        char* cppPath = "./BlurVideo";  // Path to your C++ executable

        char* arguments[] = {
            cppPath,
             option_char,
            videoPath,
            outputPath,
            NULL
        };
        
        //printf("--%s-- --%s-- --%s-- --%s--     ------------ \n", cppPath, option, videoPath, outputPath);

        /* Execute the C++ executable */
        execvp(cppPath, arguments);

        /* If execvp returns, an error occurred */
        printf("Failed to execute the C++ program.\n");
        return 1;
    } else {
        /* Parent process: Continue execution */
        printf("Parent process continues execution.\n");

        /* Wait for the child process to finish */
        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status)) {
            /* Child process exited normally */
            int exitStatus = WEXITSTATUS(status);
            printf("Child process exited with status: %d\n", exitStatus);
        } else if (WIFSIGNALED(status)) {
            /* Child process terminated by a signal*/
            int signalNumber = WTERMSIG(status);
            printf("Child process terminated by signal: %d\n", signalNumber);
        }
    }

    printf("Program execution completed.\n");
    return 0;
}
