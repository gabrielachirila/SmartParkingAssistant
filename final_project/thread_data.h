#ifndef THREAD_DATA_H
#define THREAD_DATA_H

typedef struct {
   int idThread;
   int cl; // descriptorul intors de accept
   int userId;
} thData;

#endif 
