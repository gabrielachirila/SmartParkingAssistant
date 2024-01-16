#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>

#include "client_functions.h" 
#include "response.h"
#include "constants.h"

extern int errno;

int port;                                                                                                                        

void menu_after_login(int sd) {
  int option;
  char message[MAX_MESSAGE_LENGTH] = " ";

  printf ("Now you are logged in!\n");

  while(1) {
    printf("---------------------------------------------------------\n");
    printf("Choose an option (Please enter a number between 1 and 3):\n");
    printf("1. View parking availability\n");
    printf("2. View parking history\n");
    printf("3. Logout\n");

    scanf("%d", &option);

    write(sd, &option, sizeof(int));

    if (option == 1) {
      view_parking_availability(sd);
    }
    else if (option == 2){
      view_parking_history(sd);

    }
    else if (option == 3) 
    {
      printf("Logout...\n");
      break;
    }
    else
    {
      bzero(message,MAX_MESSAGE_LENGTH);

      if (read (sd, message, MAX_MESSAGE_LENGTH) < 0)
      {
        perror ("read() error from server.\n");
      }
      printf ("%s\n", message);
    }
}
}

int main (int argc, char *argv[])
{
  int sd;			            
  struct sockaddr_in server;
  char msg[MAX_MESSAGE_LENGTH]=" ";

  if (argc != 3)
    {
      printf ("Syntax: %s <server_adress> <port>\n", argv[0]);
      return -1;
    }

  port = atoi (argv[2]);

  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("socket() error.\n");
      return errno;
    }


  server.sin_family = AF_INET;
  server.sin_addr.s_addr = inet_addr(argv[1]);
  server.sin_port = htons (port);
  
  if (connect (sd, (struct sockaddr *) &server,sizeof (struct sockaddr)) == -1)
    {
      perror ("connect() error.\n");
      return errno;
    }

  int option;

  while(1) {

    printf("Choose an option (Please enter a number between 1 and 3):\n");
    printf("1. Register\n");
    printf("2. Login\n");
    printf("3. Exit\n");

    scanf("%d", &option);

    write(sd, &option, sizeof(int));

    if(option == 1) {
        register_user(sd);
    }
    else if(option == 2){
        int ok = login_user(sd);
        if (ok == 1) {
            menu_after_login(sd);
        }
    }
    else if (option == 3)
    {
      bzero(msg,MAX_MESSAGE_LENGTH);

      if (read (sd, msg, MAX_MESSAGE_LENGTH) < 0)
      {
        perror ("read() error from server.\n");
        return errno;
      }
      printf ("%s\n", msg);
      break;
    }
    else {
      bzero(msg,MAX_MESSAGE_LENGTH);

      if (read (sd, msg, MAX_MESSAGE_LENGTH) < 0)
      {
        perror ("read() error from server.\n");
        return errno;
      }
      printf ("%s\n", msg);
    }
  }

  close (sd);
}