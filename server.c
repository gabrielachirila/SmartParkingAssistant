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

void register_user(thData tdL);
void login_user(thData tdL);
int Verific_daca_username_este_deja_folosit(char username[30]);

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
    register_user(tdL);
  }
  else if (option == 2) {
    printf("optiunea 2 \n");
    login_user(tdL);
  }

}

void register_user(thData tdL) {
  char username[30];
  char password[30];

  if (read(tdL.cl, username, sizeof(username)) <= 0) {
    printf("[Thread %d] Error reading username from client. \n", tdL.idThread);
  }

  if (read(tdL.cl, password, sizeof(password)) <= 0) {
    printf("[Thread %d] Error reading password from client. \n", tdL.idThread);
  }

  // printf("username primit: %s\n", username);
  // printf("parola primita: %s\n", password);

    if ( username == NULL || password == NULL || username[0] == '\n' || password[0] == '\n' )
        printf("Username-ul sau parola nu au fost bine definite. Completati username si/sau parola corespunzator!\n");
    else
    {
    sqlite3 *db;
    char *ErrMsg = 0;
    int rc;

    rc = sqlite3_open("proiect.db", &db);

    if(rc) 
    {
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      exit(1);
    }
    else 
      fprintf(stderr, "Opened database successfully\n");

    int success = 0;
    
    if ( Verific_daca_username_este_deja_folosit(username) == 1) 
    {
       printf("Username-ul ales este deja folosit de alt utilizator. Incercati din nou! \n");
    }
    else if ( Verific_daca_username_este_deja_folosit(username) == 0 )
    {
        char insert[200] = "INSERT INTO users (username, password) VALUES ('";
        strcat(insert, username);
        strcat(insert, "\',\'");
        strcat(insert, password);
        strcat(insert, "');");

        // printf("[server] insert from register: %s \n", insert);
        fflush(stdout);

        rc = sqlite3_exec(db, insert, 0, 0, &ErrMsg);

        if( rc != SQLITE_OK )
        {
            fprintf(stderr, "[server]SQL error: %s\n", ErrMsg);
            sqlite3_free(ErrMsg);
            printf("Eroare baza de date. Incearcati din nou!\n");
            sqlite3_close(db);
        } 
        else 
        {
            // fprintf(stdout, "[server]Records created successfully\n");
            printf("Inregistrarea s-a realizat cu succes!\n");
            sqlite3_exec(db, "COMMIT;", 0, 0, 0);
            sqlite3_close(db);
        }
    }//else_verific = 0
    else
    {
        printf("Eroare baza de date. Incearcati din nou!\n");
    }
    }

    if (write(tdL.cl, "1", sizeof(int)) <= 0) {
        perror("[Thread] Error writing to client.\n");
    }
}

int Verific_daca_username_este_deja_folosit(char username_exists[50])
{
    sqlite3 *db;
    char *ErrMsg = 0;
    sqlite3_stmt *res;
    int rc;

    rc = sqlite3_open("proiect.db", &db);

    if( rc ) 
    {
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      exit(1);
    }
    else 
      fprintf(stderr, "Opened database successfully\n");
    

    char select[200] = "SELECT id from users WHERE username=\'";
    strcat(select, username_exists);
    strcat(select, "\';");

    // printf("%s\n", select);
    fflush(stdout);

    rc = sqlite3_prepare_v2(db, select, -1, &res, 0);

    if( rc != SQLITE_OK )
        {
            return 2;
            sqlite3_close(db);
        } 
    else
    {
        int step = sqlite3_step(res);

        if (step == SQLITE_ROW)
        {
          //printf("Username-ul ales este deja folosit de altcineva. Incercati din nou!\n");
          sqlite3_close(db);
          return 1;
        }
        else 
        {
            //printf("Username-ul este ok!");
            sqlite3_close(db);
            return 0;
        }
    }

}


void login_user(thData tdL) {
  char username[30];
  char password[30];

  if (read(tdL.cl, username, sizeof(username)) <= 0) {
    printf("[Thread %d] Error reading username from client. \n", tdL.idThread);
  }

  if (read(tdL.cl, password, sizeof(password)) <= 0) {
    printf("[Thread %d] Error reading password from client. \n", tdL.idThread);
  }

if ( username == NULL || password == NULL || username[0] == '\n' || password[0] == '\n' )
        printf("Username-ul sau parola nu au fost bine definite.Completati username si parola corespunzator!\n");
   else
   {
    sqlite3 *db;
    char *ErrMsg = 0;
    sqlite3_stmt *res;
    int rc;

    rc = sqlite3_open("proiect.db", &db);

    if( rc ) 
    {
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      exit(1);
    }
    else 
      fprintf(stderr, "Opened database successfully\n");
    

    char select[200] = "SELECT id from users WHERE username=\'";
    strcat(select, username);
    strcat(select, "\' and Password=\'");
    strcat(select, password);
    strcat(select, "\';");

    // printf("%s\n", select);
    fflush(stdout);

    rc = sqlite3_prepare_v2(db, select, -1, &res, 0);

    if( rc != SQLITE_OK )
        {
            fprintf(stderr, "[server]SQL error: %s\n", ErrMsg);
            sqlite3_free(ErrMsg);
            printf("Eroare baza de date. Incearcati din nou!\n");
            sqlite3_close(db);
        } 
    else
    {
        int step = sqlite3_step(res);

        if (step == SQLITE_ROW)
        {
            // fprintf(stdout, "[server]Records updated successfully\n");
            printf("Autentificarea s-a realizat cu succes!\n");
            sqlite3_exec(db, "COMMIT;", 0, 0, 0);
            sqlite3_close(db);
        }
        else 
        {
            printf("Username/parola gresita.Incercati din nou!\n");
            sqlite3_close(db);
        }
    }
   }

  if (write(tdL.cl, "1", sizeof(int)) <= 0) {
      perror("[Thread] Error writing to client.\n");
  }
}