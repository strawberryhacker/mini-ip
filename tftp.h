// Author: strawberryhacker

#ifndef TFTP_H
#define TFTP_H

#include "utilities.h"
#include "network.h"
#include "udp.h"
#include "time.h"
#include "backoff.h"

//--------------------------------------------------------------------------------------------------

#define TFTP_MAX_FILENAME_LENGTH 64

//--------------------------------------------------------------------------------------------------

enum {
    TFTP_STATE_REQUEST,
    TFTP_STATE_READ,
    TFTP_STATE_DONE,
    TFTP_STATE_ERROR,
};

//--------------------------------------------------------------------------------------------------

typedef struct {
    Ip server_ip;

    Port client_port;
    Port server_port;

    u16 block_number;

    int state;
    Time time;
    Backoff backoff;

    char filename[TFTP_MAX_FILENAME_LENGTH];
} TftpConnection;

//--------------------------------------------------------------------------------------------------

void tftp_download_file(TftpConnection* connection, const char* filename, Ip server_ip);
void tftp_abort_download(TftpConnection* connection, const char* error_message);
int tftp_read(TftpConnection* connection, void* buffer, int size);

#endif
