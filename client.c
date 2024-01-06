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

typedef struct {
  int success;
  char message[100];
} Response;

void register_user(int sd) {
    char username[30];
    char password[30];

    bzero(username, 30);
    bzero(password, 30);

    printf("[client] Enter your username: ");
    scanf("%s", username);
    write(sd, username, sizeof(username));

    printf("[client] Enter your password: ");
    scanf("%s", password);
    write(sd, password, sizeof(password));

    Response response;
    read(sd, &response, sizeof(response));

    if (response.success) {
        printf("[client] Registration successful\n");
    } else {
        printf("[client] Registration failed: %s\n", response.message);
    }
}

int login_user(int sd) {
    char username[30];
    char password[30];

    bzero(username, 30);
    bzero(password, 30);

    printf("[client] Enter your username: ");
    scanf("%s", username);
    write(sd, username, sizeof(username));

    printf("[client] Enter your password: ");
    scanf("%s", password);
    write(sd, password, sizeof(password));

    Response response;
    read(sd, &response, sizeof(response));

    if (response.success) {
        printf("[client] Login successful\n");
        return 1;
    } else {
        printf("[client] Login failed: %s\n", response.message);
        return 0;
    }
}

void menu_after_login(int sd) {
    int option1;

    printf ("Acum sunteti autentificat!\n");

    while(1) {

      printf("[client] Choose an option:\n");
      printf("1. View parking availability\n");
      printf("2. View parking history\n");
      printf("3. Find nearest parking spot\n");
      printf("4. Logout\n");

      scanf("%d", &option1);

      write(sd, &option1, sizeof(int));

      if (option1 == 1) {
          char message[200];
          bzero(message, 200);

          // Option 1: View parking availability
          char city[50];
          char area[100];

          char cityList[500];
          bzero(cityList, 500);
          if (read(sd, cityList, sizeof(cityList)) <= 0) {
              perror("[client] Error reading cities from server.\n");
              return;
          }

          printf("[client] Choose a city from this list where you want to park: \n %s \n", cityList);

          // Step 1: Choose a city
          printf("Enter the city where you want to park: ");
          scanf("%s", city);

          // Send the selected city to the server
          write(sd, city, sizeof(city));

          char areaList[200];
          bzero(areaList, 200);
          if (read(sd, areaList, sizeof(areaList)) <= 0) {
              perror("[client] Error reading areas from server.\n");
              return;
          }

          printf("[client] Choose an area in city %s from this list where you want to park: \n %s \n", city, areaList);

          // Step 2: Choose an area
          printf("Enter the area in city %s where you want to park: ", city);
          fflush (stdout);
          read (0, area, sizeof(area));

          size_t len = strlen(area);
          if (len > 0 && area[len - 1] == '\n') {
              area[len - 1] = '\0';
          }

          printf("area: %s \n", area);

          // Send the selected area to the server
          write(sd, area, sizeof(area));

          // Step 3: Receive and display available parking spots
          read(sd, &message, sizeof(message));
          printf("[client] Mesajul primit este: %s\n", message);
      }
      else if (option1 == 2){
        printf("[client]  View parking history...\n");
      }
      else if (option1 == 3)
      {
        printf("[client] Find nearest parking spot...\n");
      }
      else if (option1 == 4) 
      {
        printf("[client] Logout...\n");
        break;
      }
      else
      {
        printf("Invalid option \n");
      }

  }
}

int main (int argc, char *argv[])
{
  int sd;			            // descriptorul de socket
  struct sockaddr_in server;	// structura folosita pentru conectare 
  char msg[200]=" ";

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
      int ok = login_user(sd);
      if (ok == 1) {
          // Call the menu function after a successful login
          menu_after_login(sd);
      }
  }
  else if (option == 3) //iesire din aplicatie
  {
    bzero(msg,200);
    if (read (sd, msg, 200) < 0)
    {
      perror ("[client]Eroare la read() de la server.\n");
      return errno;
    }
    printf ("[client]Mesajul primit este: %s\n", msg);
    // printf ("Iesire din aplicatie\n");
    break;
  }
  else {
    bzero(msg,200);
    if (read (sd, msg, 200) < 0)
    {
      perror ("[client]Eroare la read() de la server.\n");
      return errno;
    }
    /* afisam mesajul primit */
    printf ("[client]Mesajul primit este: %s\n", msg);
    // printf ("Invalid option. Choose a valid option\n");
  }

  }

  /* inchidem conexiunea, am terminat */
  close (sd);
}