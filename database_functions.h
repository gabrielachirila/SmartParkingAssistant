#ifndef DATABASE_FUNCTIONS_H
#define DATABASE_FUNCTIONS_H

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

#include "response.h"
#include "constants.h"

void createTables(sqlite3 *db);

#endif 