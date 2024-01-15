#ifndef RESPONSE_H
#define RESPONSE_H

typedef struct {
    int success;
    char message[MAX_MESSAGE_LENGTH];
} Response;

#endif