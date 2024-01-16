#ifndef RESPONSE_H
#define RESPONSE_H

#include "constants.h"

typedef struct {
    int success;
    char message[MAX_MESSAGE_LENGTH];
} Response;

#endif