/* servTCPConcTh2.c - Exemplu de server TCP concurent care deserveste clientii
   prin crearea unui thread pentru fiecare client.
   Asteapta un numar de la clienti si intoarce clientilor numarul incrementat.
	Intoarce corect identificatorul din program al thread-ului.
  
   
   Autor: Lenuta Alboaie  <adria@info.uaic.ro> (c)
*/

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


/* portul folosit */
#define PORT 2908

/* codul de eroare returnat de anumite apeluri */
extern int errno;

typedef struct thData{
	int idThread; //id-ul thread-ului tinut in evidenta de acest program
	int cl; //descriptorul intors de accept
} thData;

static void *treat(void *); /* functia executata de fiecare thread ce realizeaza comunicarea cu clientii */
void raspunde(void *);

void register_user(sqlite3 *db, thData tdL);
void login_user(sqlite3 *db, thData tdL);

sqlite3 *db;



int main ()
{
  struct sockaddr_in server;	// structura folosita de server
  struct sockaddr_in from;	
  int nr;		//mesajul primit de trimis la client 
  int sd;		//descriptorul de socket 
  int pid;
  pthread_t th[100];    //Identificatorii thread-urilor care se vor crea
	int i=0;

  sqlite3 *db;
  char *error_message = 0;

  if (sqlite3_open("proiect.db", &db) != SQLITE_OK) {
    fprintf(stderr, "Can't open database: %s \n", sqlite3_errmsg(db));
    return -1;
  }

  char *createTableQuery = "CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY AUTOINCREMENT, username TEXT, password TEXT);";
  if (sqlite3_exec(db, createTableQuery, 0, 0, &error_message) != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s \n", error_message);
    sqlite3_free(error_message);
    sqlite3_close(db);
    return -1;
  }

  /* crearea unui socket */
  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("[server]Eroare la socket().\n");
      return errno;
    }
  /* utilizarea optiunii SO_REUSEADDR */
  int on=1;
  setsockopt(sd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
  
  /* pregatirea structurilor de date */
  bzero (&server, sizeof (server));
  bzero (&from, sizeof (from));
  
  /* umplem structura folosita de server */
  /* stabilirea familiei de socket-uri */
    server.sin_family = AF_INET;	
  /* acceptam orice adresa */
    server.sin_addr.s_addr = htonl (INADDR_ANY);
  /* utilizam un port utilizator */
    server.sin_port = htons (PORT);
  
  /* atasam socketul */
  if (bind (sd, (struct sockaddr *) &server, sizeof (struct sockaddr)) == -1)
    {
      perror ("[server]Eroare la bind().\n");
      return errno;
    }

  /* punem serverul sa asculte daca vin clienti sa se conecteze */
  if (listen (sd, 2) == -1)
    {
      perror ("[server]Eroare la listen().\n");
      return errno;
    }
  /* servim in mod concurent clientii...folosind thread-uri */
  while (1)
    {
      int client;
      thData * td; //parametru functia executata de thread     
      int length = sizeof (from);

      printf ("[server]Asteptam la portul %d...\n",PORT);
      fflush (stdout);

      // client= malloc(sizeof(int));
      /* acceptam un client (stare blocanta pina la realizarea conexiunii) */
      if ( (client = accept (sd, (struct sockaddr *) &from, &length)) < 0)
	{
	  perror ("[server]Eroare la accept().\n");
	  continue;
	}
	
        /* s-a realizat conexiunea, se astepta mesajul */
    
	// int idThread; //id-ul threadului
	// int cl; //descriptorul intors de accept

	td=(struct thData*)malloc(sizeof(struct thData));	
	td->idThread=i++;
	td->cl=client;

	pthread_create(&th[i], NULL, &treat, td);	   
				
	}//while    
};				
static void *treat(void * arg)
{		
		struct thData tdL; 
		tdL= *((struct thData*)arg);	
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
	struct thData tdL; 
	tdL= *((struct thData*)arg);
	if (read (tdL.cl, &option, sizeof(int)) <= 0)
			{
			  printf("[Thread %d]\n",tdL.idThread);
			  perror ("Eroare la read() de la client.\n");
			
			}
	
	printf ("[Thread %d]Mesajul a fost receptionat...Optiunea %d\n",tdL.idThread, option);

  if (option == 1) {
    printf("optiunea 1 \n");
    register_user(db, tdL);
  }
  else if (option == 2) {
    printf("optiunea 2 \n");
    login_user(db, tdL);
  }

}

void register_user(sqlite3 *db, thData tdL) {
  char username[30];
  char password[30];

  if (read(tdL.cl, username, sizeof(username)) <= 0) {
    printf("[Thread %d] Error reading username from client. \n", tdL.idThread);
  }

  if (read(tdL.cl, password, sizeof(password)) <= 0) {
    printf("[Thread %d] Error reading password from client. \n", tdL.idThread);
  }

  printf("username primit: %s\n", username);
  printf("parola primita: %s\n", password);

  char query[200];
  snprintf(query, sizeof(query), "INSERT INTO users (username, password) VALUES ('%s', '%s');", username, password);

  sqlite3_stmt *stmt;
  int success = 0;

  if (sqlite3_prepare_v2(db, query, -1, &stmt, 0) == SQLITE_OK) {
      sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
      sqlite3_bind_text(stmt, 2, password, -1, SQLITE_STATIC);

      if (sqlite3_step(stmt) == SQLITE_DONE) {
          success = 1;
      } else {
          fprintf(stderr, "[Thread %d] SQL error: %s \n", tdL.idThread, sqlite3_errmsg(db));
      }

      sqlite3_finalize(stmt);
  } else {
      fprintf(stderr, "[Thread %d] SQL error: %s \n", tdL.idThread, sqlite3_errmsg(db));
  }

  if (write(tdL.cl, &success, sizeof(int)) <= 0) {
      perror("[Thread] Error writing to client.\n");
  }
}

void login_user(sqlite3 *db, thData tdL) {
  char username[30];
  char password[30];

  if (read(tdL.cl, username, sizeof(username)) <= 0) {
    printf("[Thread %d] Error reading username from client. \n", tdL.idThread);
  }

  if (read(tdL.cl, password, sizeof(password)) <= 0) {
    printf("[Thread %d] Error reading password from client. \n", tdL.idThread);
  }

  char query[200];
  snprintf(query, sizeof(query), "SELECT * FROM users WHERE username='%s' AND password='%s';", username, password);

  int success = 0;
  char *error_message = 0;

  if(sqlite3_exec(db, query, 0, &success, &error_message) != SQLITE_OK) {
    fprintf(stderr, "[Thread %d] SQL error: %s \n", tdL.idThread, error_message);
    sqlite3_free(error_message);
  }
  else {
    success = 1;
  }

  printf("success: %d\n",success);

  if (write(tdL.cl, &success, sizeof(int)) <= 0) {
      perror("[Thread] Error writing to client.\n");
  }
}