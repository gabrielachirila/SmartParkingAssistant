#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <stdint.h>
#include <sqlite3.h>

#include "server_functions.h" 
#include "database_functions.h"
#include "response.h"
#include "constants.h"
#include "thread_data.h"

#define PORT 2908

extern int errno;

static void *treat(void *); /* functia executata de fiecare thread ce realizeaza comunicarea cu clientii */
void raspunde(void *);

int main ()
{
  struct sockaddr_in server;
  struct sockaddr_in from;	
  int sd;		
  int pid;
  pthread_t th[100];    //Identificatorii thread-urilor care se vor crea
	int i=0;
  char *error_message = 0;

  srand(time(NULL));

  sqlite3 *db = open_database_connection();
  createTables(db);
  close_database_connection(db);

  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("[server]Eroare la socket().\n");
      return errno;
    }

  int on=1;
  setsockopt(sd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));

  bzero (&server, sizeof (server));
  bzero (&from, sizeof (from));

  server.sin_family = AF_INET;	
  server.sin_addr.s_addr = htonl (INADDR_ANY);
  server.sin_port = htons (PORT);
  
  if (bind (sd, (struct sockaddr *) &server, sizeof (struct sockaddr)) == -1)
    {
      perror ("[server]Eroare la bind().\n");
      return errno;
    }

  if (listen (sd, 2) == -1)
    {
      perror ("[server]Eroare la listen().\n");
      return errno;
    }

  while (1)
    {
      int client;
      thData * td;   
      int length = sizeof (from);

      printf ("[server]Asteptam la portul %d...\n",PORT);
      fflush (stdout);

      if ( (client = accept (sd, (struct sockaddr *) &from, &length)) < 0)
        {
          perror ("[server]Eroare la accept().\n");
          continue;
        }

      td = (thData*)malloc(sizeof(thData));
      td->idThread = i++;
      td->cl = client;

      pthread_create(&th[i], NULL, &treat, td);	   
				
	  } 
};	

static void *treat(void * arg)
{		
		thData tdL = *((thData*)arg);
		printf ("[Thread %d] - Asteptam mesajul...\n", tdL.idThread);
		fflush (stdout);		 
		pthread_detach(pthread_self());		
		raspunde((struct thData*)arg);
		/* am terminat cu acest client, inchidem conexiunea */
		close ((intptr_t)arg);
		return(NULL);	
  		
};


void raspunde(void *arg)
{
  int option, i=0;
  thData tdL = *((thData*)arg);

  char msgrasp[MAX_MESSAGE_LENGTH]=" ";

  while(1) {
    
    if (read (tdL.cl, &option, sizeof(int)) <= 0)
      {
        printf("[Thread %d]\n",tdL.idThread);
        perror ("Eroare la read() de la client.\n");
      
      }

    printf ("[Thread %d]Mesajul a fost receptionat...Optiunea %d\n",tdL.idThread, option);

    if (option == 1) {
      printf("Optiunea 1 \n");
      register_user(tdL);
    } 
    else if (option == 2) {
      printf("Optiunea 2 \n");
      int ok = login_user(&tdL);

      if (ok == 1) {
        int option1;

        while(1) {

          // dupa ce autentificarea s-a realizat cu succes, trimitem informatii dupa logare
          if (read (tdL.cl, &option1, sizeof(int)) <= 0)
          {
            printf("[Thread %d]\n",tdL.idThread);
            perror ("Eroare la read() de la client.\n");
          }

          if (option1 == 1)
          {
            printf("[Thread %d] Optiunea %d: View parking availability \n",tdL.idThread, option1);
            viewParkingAvailability(tdL);
          }
          else if (option1 == 2)
          {
            printf("[Thread %d] Optiunea %d: View parking history \n",tdL.idThread, option1);
            viewParkingHistory(tdL);
          }
          else if(option1 == 3)
          {
            printf("[Thread %d] Optiunea %d: Logout \n",tdL.idThread, option1);
            break;
          }
          else 
          {
            printf("[Thread %d] Optiune invalida. Incercati din nou\n", tdL.idThread);

            bzero(msgrasp,MAX_MESSAGE_LENGTH);

            strcat(msgrasp,"Optiune invalida. Incercati din nou\n");

            if (write (tdL.cl , msgrasp, MAX_MESSAGE_LENGTH) <= 0)
            {
              printf("[Thread %d] ",tdL.idThread);
              perror ("[Thread]Eroare la write() catre client.\n");
            }
            else
              printf ("[Thread %d]Mesajul a fost trasmis cu succes.\n",tdL.idThread);	
          }

        }
      }
    }
    else if (option == 3) {
        printf("Optiunea 3 \n");
        printf("[Thread %d] Clientul a iesit din aplicatie\n", tdL.idThread);

        bzero(msgrasp,MAX_MESSAGE_LENGTH);
        strcat(msgrasp,"Exit app..\n");

        if (write (tdL.cl , msgrasp, MAX_MESSAGE_LENGTH) <= 0)
        {
          printf("[Thread %d] ",tdL.idThread);
          perror ("[Thread]Eroare la write() catre client.\n");
        }
        else
          printf ("[Thread %d]Mesajul a fost trasmis cu succes.\n",tdL.idThread);	
          break;
    }
    else {
        printf("[Thread %d] Optiune invalida. Incercati din nou\n", tdL.idThread);

        bzero(msgrasp,MAX_MESSAGE_LENGTH);
        strcat(msgrasp,"Optiune invalida. Incercati din nou\n");


        if (write (tdL.cl , msgrasp, MAX_MESSAGE_LENGTH) <= 0)
        {
          printf("[Thread %d] ",tdL.idThread);
          perror ("[Thread]Eroare la write() catre client.\n");
        }
        else
          printf ("[Thread %d]Mesajul a fost trasmis cu succes.\n",tdL.idThread);	
    }
  }
}




