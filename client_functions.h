#ifndef CLIENT_FUNCTIONS_H
#define CLIENT_FUNCTIONS_H

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>  
#include <strings.h> 

#include "response.h"
#include "constants.h"

void register_user(int sd);
int login_user(int sd);
void view_parking_availability(int sd);
void view_parking_history(int sd);

#endif
