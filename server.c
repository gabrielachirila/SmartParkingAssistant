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

void register_user(thData tdL, sqlite3 *db);
void login_user(thData tdL, sqlite3 *db);
int verify_username(char username[30],  sqlite3 *db);

typedef struct {
  int success;
  char message[100];
} Response;

static void *treat(void *); /* functia executata de fiecare thread ce realizeaza comunicarea cu clientii */
void raspunde(void *);

sqlite3 *open_database_connection() {
  sqlite3 *db;
  int rc;

  rc = sqlite3_open("proiect.db", &db);

  if (rc) {
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    exit(1);
  } else {
    fprintf(stderr, "Opened database successfully\n");
  }

  return db;
}

void close_database_connection(sqlite3 *db) {
  sqlite3_close(db);
}

int main ()
{
  struct sockaddr_in server;	// structura folosita de server
  struct sockaddr_in from;	
  int nr;		//mesajul primit de trimis la client 
  int sd;		//descriptorul de socket 
  int pid;
  pthread_t th[100];    //Identificatorii thread-urilor care se vor crea
	int i=0;

  sqlite3 *db = open_database_connection();
  char *error_message = 0;

  char *createTableQuery = "CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY AUTOINCREMENT, username TEXT, password TEXT);";
  if (sqlite3_exec(db, createTableQuery, 0, 0, &error_message) != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s \n", error_message);
    sqlite3_free(error_message);
    return -1;
  }

  close_database_connection(db);

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

  // Open the database connection
    sqlite3 *db = open_database_connection();

    // Process the client request based on the received option
    if (option == 1) {
        printf("Optiunea 1 \n");
        register_user(tdL, db);
    } else if (option == 2) {
        printf("Optiunea 2 \n");
        login_user(tdL, db);
    }

    // Close the database connection
    close_database_connection(db);

}

void register_user(thData tdL, sqlite3 *db) {
  char username[30];
  char password[30];

  bzero(username, 30);
  bzero(password, 30);

  if (read(tdL.cl, username, sizeof(username)) <= 0) {
    printf("[Thread %d] Error reading username from client. \n", tdL.idThread);
  }

  if (read(tdL.cl, password, sizeof(password)) <= 0) {
    printf("[Thread %d] Error reading password from client. \n", tdL.idThread);
  }

  Response response;
  response.success = 0; // Default to failure
  printf("username primit: %s\n", username);
  printf("parola primita: %s\n", password);

  if (username == NULL || password == NULL || username[0] == '\n' || password[0] == '\n') {
    printf("Username-ul sau parola nu au fost bine definite. Completati username si/sau parola corespunzator!\n");
    snprintf(response.message, sizeof(response.message), "Username-ul sau parola nu au fost bine definite. Completati username si/sau parola corespunzator!");
  } else {
    char *ErrMsg = 0;
    int rc;

    if (verify_username(username, db) == 0) {
      printf("Username-ul ales este deja folosit de alt utilizator. Incercati din nou! \n");
      snprintf(response.message, sizeof(response.message), "Username-ul %s este deja folosit de alt utilizator. Incercati altul!", username);
    } else if (verify_username(username, db) == 1) {
      char *insert_query = "INSERT INTO users (username, password) VALUES (?, ?)";
      sqlite3_stmt *stmt;

      rc = sqlite3_prepare_v2(db, insert_query, -1, &stmt, 0);

      if (rc != SQLITE_OK) {
        fprintf(stderr, "[server]SQL error: %s\n", sqlite3_errmsg(db));
        snprintf(response.message, sizeof(response.message), "Eroare baza de date. Incearcati din nou!");
      } else {
        rc = sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
        if (rc != SQLITE_OK) {
          fprintf(stderr, "[server]SQL error: %s\n", sqlite3_errmsg(db));
          sqlite3_finalize(stmt);
          snprintf(response.message, sizeof(response.message), "Eroare la bind_text. Incearcati din nou!");
        } else {
          rc = sqlite3_bind_text(stmt, 2, password, -1, SQLITE_STATIC);
          if (rc != SQLITE_OK) {
            fprintf(stderr, "[server]SQL error: %s\n", sqlite3_errmsg(db));
            sqlite3_finalize(stmt);
            snprintf(response.message, sizeof(response.message), "Eroare la bind_text. Incearcati din nou!");
          } else {
            rc = sqlite3_step(stmt);

            if (rc != SQLITE_DONE) {
              fprintf(stderr, "[server]SQL error: %s\n", sqlite3_errmsg(db));
              sqlite3_finalize(stmt);
              snprintf(response.message, sizeof(response.message), "Eroare la inserarea in baza de date. Incearcati din nou!");
            } else {
              sqlite3_finalize(stmt);
              sqlite3_exec(db, "COMMIT;", 0, 0, 0);
              response.success = 1;
              snprintf(response.message, sizeof(response.message), "Inregistrarea s-a realizat cu succes!");
            }
          }
        }
      }
    } else {
      printf("Eroare baza de date. Incearcati din nou!\n");
      snprintf(response.message, sizeof(response.message), "Eroare baza de date. Incearcati din nou!");
    }
  }

  // Send the response structure to the client
  if (write(tdL.cl, &response, sizeof(Response)) <= 0) {
    perror("[Thread] Error writing to client.\n");
  }

  if (response.success) {
    printf("%s\n", response.message);
  }
}

int verify_username(char username_exists[30], sqlite3 *db)
{
    char *ErrMsg = 0;
    sqlite3_stmt *res;
    int rc;
    
    char select[200] = "SELECT id from users WHERE username=\'";
    strcat(select, username_exists);
    strcat(select, "\';");

    // printf("%s\n", select);
    fflush(stdout);

    rc = sqlite3_prepare_v2(db, select, -1, &res, 0);

    if( rc != SQLITE_OK )
        {
            return 2; // eroare la select
        } 
    else
    {
        int step = sqlite3_step(res);

        if (step == SQLITE_ROW)
        {
          return 0; // am gasit un utilizator cu acest username
        }
        else 
        {
          return 1; // nu s-a gasit niciun utilizator cu acest username
        }
    }

}


void login_user(thData tdL, sqlite3 *db) {
    char username[30];
    char password[30];

    bzero(username, 30);
    bzero(password, 30);

    if (read(tdL.cl, username, sizeof(username)) <= 0) {
        printf("[Thread %d] Error reading username from client. \n", tdL.idThread);
    }

    if (read(tdL.cl, password, sizeof(password)) <= 0) {
        printf("[Thread %d] Error reading password from client. \n", tdL.idThread);
    }

    Response response;
    response.success = 0; // Default to failure
    printf("username primit: %s\n", username);
    printf("parola primita: %s\n", password);

    if (username == NULL || password == NULL || username[0] == '\n' || password[0] == '\n') {
        printf("Username-ul sau parola nu au fost bine definite. Completati username si/sau parola corespunzator!\n");
        snprintf(response.message, sizeof(response.message), "Username-ul sau parola nu au fost bine definite. Completati username si/sau parola corespunzator!");
    } else {
        char *ErrMsg = 0;
        sqlite3_stmt *res;
        int rc;

        char select[200] = "SELECT id from users WHERE username=\'";
        strcat(select, username);
        strcat(select, "\' and Password=\'");
        strcat(select, password);
        strcat(select, "\';");

        rc = sqlite3_prepare_v2(db, select, -1, &res, 0);

        if (rc != SQLITE_OK) {
            fprintf(stderr, "[server]SQL error: %s\n", sqlite3_errmsg(db));
            snprintf(response.message, sizeof(response.message), "Eroare baza de date. Incearcati din nou!");
        } else {
            int step = sqlite3_step(res);

            if (step == SQLITE_ROW) {
                response.success = 1;
                snprintf(response.message, sizeof(response.message), "Autentificarea s-a realizat cu succes!");
            } else {
                snprintf(response.message, sizeof(response.message), "Username/parola gresita. Incercati din nou!");
            }
        }
    }

    // Send the response structure to the client
    if (write(tdL.cl, &response, sizeof(Response)) <= 0) {
        perror("[Thread] Error writing to client.\n");
    }

    if (response.success) {
        printf("%s\n", response.message);
    }
}
