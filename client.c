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
    scanf("%[^\n]s", username);  // %[^\n]s allows spaces in the input
    write(sd, username, sizeof(username));

    printf("[client] Enter your password: ");
    scanf(" %[^\n]s", password);  // %[^\n]s allows spaces in the input
    write(sd, password, sizeof(password));

    Response response;
    read(sd, &response, sizeof(Response));

    if (response.success) {
        printf("[client] Registration successful\n");
    } else {
        printf("[client] Registration failed: %s\n", response.message);
    }
}

void login_user(int sd, int ok) {
    char username[30];
    char password[30];

    printf("[client] Enter your username: ");
    scanf("%s", username);
    write(sd, username, sizeof(username));

    printf("[client] Enter your password: ");
    scanf("%s", password);
    write(sd, password, sizeof(password));

    Response response;
    read(sd, &response, sizeof(Response));

    if (response.success) {
        printf("[client] Login successful\n");
        ok = 1;
    } else {
        printf("[client] Login failed: %s\n", response.message);
    }
}

void menu_after_login(int sd) {
    int option1;

    do {
        printf ("Acum sunteti autentificat!\n");
        printf("[client] Choose an option:\n");
        printf("1. Find nearest parking spot\n");
        printf("2. View parking history\n");
        printf("3. View parking availability\n");
        printf("2. Logout\n");

        scanf("%d", &option1);
        write(sd, &option1, sizeof(int));

        switch (option1) {
            case 1:
                // Implement logout functionality
                printf("[client] Logout successful\n");
                break;
            case 2:
                // Implement see parking spots functionality
                printf("[client] Showing parking spots...\n");
                break;
            default:
                printf("[client] Invalid option. Please try again.\n");
        }
    } while (option != 3);
}

int main (int argc, char *argv[])
{
  int sd;			            // descriptorul de socket
  struct sockaddr_in server;	// structura folosita pentru conectare 
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

  while(1) {

  int option;
  printf("[client] Choose an option:\n");
  printf("1. Register\n");
  printf("2. Login\n");
  printf("3. Exit\n");

  // citim optiunea aleasa de client inainte de a se conecta
  scanf("%d", &option);

  // trimitem catre server optiunea aleasa de client 
  write(sd, &option, sizeof(int));

  if(option == 1) {
      register_user(sd);
  }
  else if(option == 2){
      int ok = 0;
      login_user(sd, ok);
      if (ok == 1) {
          // Call the menu function after a successful login
          menu_after_login(sd);
      }
  }
  else if (option == 3) //iesire din aplicatie
  {
    printf ("Iesire din aplicatie\n");
    break;
  }

  }

  /* inchidem conexiunea, am terminat */
  close (sd);
}