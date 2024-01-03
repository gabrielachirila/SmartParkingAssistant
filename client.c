/* cliTCPIt.c - Exemplu de client TCP
   Trimite un numar la server; primeste de la server numarul incrementat.
         
   Autor: Lenuta Alboaie  <adria@info.uaic.ro> (c)
*/
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>

/* codul de eroare returnat de anumite apeluri */
extern int errno;

/* portul de conectare la server*/
int port;

void register_user(int sd) {
    char username[30];
    char password[30];

    printf("[client] Enter your username: ");
    scanf("%s", username);
    write(sd, username, sizeof(username));

    printf("[client] Enter your password: ");
    scanf("%s", password);
    write(sd, password, sizeof(password));

    int success;
    read(sd, &success, sizeof(int));

    if (success) {
        printf("[client] Registration successful. \n");
    }
    else {
        printf("[client] Registration failed. Username already taken. \n");
    }
}

void login_user(int sd) {
    char username[30];
    char password[30];

    printf("[client] Enter your username: ");
    scanf("%s", username);
    write(sd, username, sizeof(username));

    printf("[client] Enter your password: ");
    scanf("%s", password);
    write(sd, password, sizeof(password));

    int success;
    read(sd, &success, sizeof(int));

        if (success) {
        printf("[client] Login successful. \n");
    }
    else {
        printf("[client] Login failed. Invalid username or password. \n");
    }
    
}

int main (int argc, char *argv[])
{
  int sd;			// descriptorul de socket
  struct sockaddr_in server;	// structura folosita pentru conectare 
  		// mesajul trimis
  int nr=0;
  char buf[10];

  /* exista toate argumentele in linia de comanda? */
  if (argc != 3)
    {
      printf ("Sintaxa: %s <adresa_server> <port>\n", argv[0]);
      return -1;
    }

  /* stabilim portul */
  port = atoi (argv[2]);

  /* cream socketul */
  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("Eroare la socket().\n");
      return errno;
    }

  /* umplem structura folosita pentru realizarea conexiunii cu serverul */
  /* familia socket-ului */
  server.sin_family = AF_INET;
  /* adresa IP a serverului */
  server.sin_addr.s_addr = inet_addr(argv[1]);
  /* portul de conectare */
  server.sin_port = htons (port);
  
  /* ne conectam la server */
  if (connect (sd, (struct sockaddr *) &server,sizeof (struct sockaddr)) == -1)
    {
      perror ("[client]Eroare la connect().\n");
      return errno;
    }

    int option;
    printf("[client] Choose an option:\n");
    printf("1. Register\n");
    printf("2. Login\n");
    scanf("%d", &option);
    write(sd, &option, sizeof(int));

    if(option == 1){
        register_user(sd);
    }
    else if(option == 2){
        login_user(sd);
    }

  /* inchidem conexiunea, am terminat */
  close (sd);
}
