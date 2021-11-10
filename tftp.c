// Author: strawberryhacker

#include "tftp.h"

//--------------------------------------------------------------------------------------------------

#define TFTP_BACKOFF_START_TIMEOUT   500
#define TFTP_BACKOFF_MAX_TIMEOUT     10000
#define TFTP_BACKOFF_JITTER_FRACTION 4

#define TFTP_INITIAL_SERVER_PORT 69
#define TFTP_CLIENT_PORT         23456

//--------------------------------------------------------------------------------------------------

enum {
    TFTP_OPCODE_READ_REQUEST  = 1,
    TFTP_OPCODE_WRITE_REQUEST = 2,
    TFTP_OPCODE_DATA          = 3,
    TFTP_OPCODE_ACK           = 4,
    TFTP_OPCODE_ERROR         = 5,
    TFTP_OPCODE_OACK          = 6,
};

enum {
    TFTP_ERROR_NOT_DEFINED        = 0,
    TFTP_ERROR_FILE_NOT_FOUND     = 1,
    TFTP_ERROR_ACCESS_VIOLATION   = 2,
    TFTP_ERROR_DISK_FULL          = 3,
    TFTP_ERROR_ILLEGAL_OPERATION  = 4,
    TFTP_ERROR_UNKNOWN_TID        = 5,
    TFTP_ERROR_FILE_ALREADY_EXIST = 6,
    TFTP_ERROR_NO_SUCH_USER       = 7,
};

enum {
    TFTP_STATUS_OK,
    TFTP_STATUS_ERROR,
    TFTP_STATUS_RETRY,
};

//--------------------------------------------------------------------------------------------------

typedef struct {
    u16 opcode;
    u16 block_number;
} TftpDataHeader;

//--------------------------------------------------------------------------------------------------

void tftp_download_file(TftpConnection* connection, const char* filename, Ip server_ip) {
    connection->client_port = TFTP_CLIENT_PORT;
    connection->server_port = TFTP_INITIAL_SERVER_PORT;

    connection->server_ip = server_ip;
    connection->state = TFTP_STATE_REQUEST;
    connection->block_number = 0;

    udp_listen(connection->client_port, 1);
    backoff_init(&connection->backoff, TFTP_BACKOFF_START_TIMEOUT, TFTP_BACKOFF_MAX_TIMEOUT, TFTP_BACKOFF_JITTER_FRACTION);

    for (int i = 0; i < TFTP_MAX_FILENAME_LENGTH && filename[i]; i++) {
        connection->filename[i] = filename[i];

        if (filename[i] == 0) {
            break;
        }
    }
}

//--------------------------------------------------------------------------------------------------

// The data pointer will be moved to the first character after the zero.
void add_string_followed_by_zero(const char* string, u8** data) {
    char* dest = *data;

    while (*string) {
        *dest++ = *string++;
    }

    *dest++ = 0;
    *data = dest;
}

//--------------------------------------------------------------------------------------------------

void send_tftp_request(TftpConnection* connection) {
    NetworkPacket* packet = allocate_network_packet();
    u8* data_start = (u8 *)&packet->data[packet->index];
    u8* data = data_start;

    write_be16(TFTP_OPCODE_READ_REQUEST, data);
    data += 2;

    add_string_followed_by_zero(connection->filename, &data);
    add_string_followed_by_zero("octet", &data);
    add_string_followed_by_zero("blksize", &data);
    add_string_followed_by_zero("512", &data);

    packet->length = data - data_start;
    udp_send_zero_copy(packet, connection->client_port, connection->server_port, connection->server_ip);
}

//--------------------------------------------------------------------------------------------------

void ack_current_block(TftpConnection* connection) {
    NetworkPacket* packet = allocate_network_packet();
    u8* data = (u8 *)&packet->data[packet->index];

    write_be16(TFTP_OPCODE_ACK, data);
    data += 2;

    write_be16(connection->block_number, data);
    data += 2;

    packet->length = 4;
    udp_send_zero_copy(packet, connection->client_port, connection->server_port, connection->server_ip);
}

//--------------------------------------------------------------------------------------------------

static void send_error_to(TftpConnection* connection, u16 dest_port, u16 error_code, const char* error_message) {
    NetworkPacket* packet = allocate_network_packet();
    u8* data_start = (u8 *)&packet->data[packet->index];
    u8* data = data_start;

    write_be16(TFTP_OPCODE_ERROR, data);
    data += 2;

    write_be16(error_code, data);
    data += 2;

    add_string_followed_by_zero(error_message, &data);

    packet->length = data - data_start;
    udp_send_zero_copy(packet, connection->client_port, dest_port, connection->server_ip);
}

//--------------------------------------------------------------------------------------------------

static void send_error(TftpConnection* connection, u16 error_code, const char* error_message) {
    send_error_to(connection, connection->server_port, error_code, error_message);
}

//--------------------------------------------------------------------------------------------------

bool compare_option(char** data, char* end, const char* string) {
    char* source = *data;

    while (source != end && *string) {
        if (*string++ != *source++) {
            return false;
        }
    }

    *data = source + 1; // Skip the trailing zero.
    return source != end && *source == 0;
}

//--------------------------------------------------------------------------------------------------

int try_read_oack(TftpConnection* connection) {
    NetworkPacket* packet = udp_receive_zero_copy(connection->client_port);

    if (packet == 0 || packet->length < 2) {
        return TFTP_STATUS_RETRY;
    }


    int status = TFTP_STATUS_OK;
    char* data = (u8 *)&packet->data[packet->index];
    char* end = data + packet->length;

    if (read_be16(data) != TFTP_OPCODE_OACK) {
        status = TFTP_STATUS_ERROR;
        goto return_and_delete;
    }

    // Skip the OACK opcode.
    data += 2;

    if (compare_option(&data, end, "blksize") == false) {
        status = TFTP_STATUS_ERROR;
        goto return_and_delete;
    }

    if (compare_option(&data, end, "512") == false) {
        status = TFTP_STATUS_ERROR;
        goto return_and_delete;
    }

    connection->server_port = packet->source_port;

    return_and_delete:
    free_network_packet(packet);
    return status;
}

//--------------------------------------------------------------------------------------------------


static int try_read_data(TftpConnection* connection, void* data, int size) {
    NetworkPacket* packet = udp_receive_zero_copy(connection->client_port);
    if (packet == 0) {
        return 0;
    }

    TftpDataHeader* header = (TftpDataHeader *)&packet->data[packet->index];
    u16 opcode = read_be16(&header->opcode);
    u16 block_number = read_be16(&header->block_number);

    int bytes_written = 0;

    if (connection->server_port != TFTP_INITIAL_SERVER_PORT && connection->server_port != packet->source_port) {
        // Mentioned in the RFC. This is the case where the initial request was duplicated in some way. 
        // The server replies to our machine with two port numbers.
        send_error_to(connection, packet->source_port, TFTP_ERROR_UNKNOWN_TID, "wrong port");
    }
    else if (block_number == connection->block_number) {
        // Last ACK was lost.
        ack_current_block(connection);
    }
    else if (block_number == (connection->block_number + 1)) {
        size = limit(size, packet->length - sizeof(TftpDataHeader));
        connection->block_number = block_number;
        memory_copy((u8 *)header + sizeof(TftpDataHeader), data, size);
        bytes_written = size;
    }

    free_network_packet(packet);
    return bytes_written;
}

//--------------------------------------------------------------------------------------------------

int tftp_read(TftpConnection* connection, void* buffer, int size) {
    if (connection->state == TFTP_STATE_REQUEST) {
        if (backoff_timeout(&connection->backoff)) {
            send_tftp_request(connection);
            next_backoff(&connection->backoff);
        }

        int status = try_read_oack(connection);

        if (status == TFTP_STATUS_OK) {
            backoff_reset(&connection->backoff);
            connection->state = TFTP_STATE_READ;

            // Block number starts at zero because the first ACK should be for the request.
            connection->block_number = 0;
        }

        if (status == TFTP_STATUS_ERROR) {
            connection->state = TFTP_STATE_ERROR;
        }
    }
    else if (connection->state == TFTP_STATE_READ) {
        if (backoff_timeout(&connection->backoff)) {
            ack_current_block(connection);
            next_backoff(&connection->backoff);
        }

        int count = try_read_data(connection, buffer, size);
        
        if (count) {
            backoff_reset(&connection->backoff);

            if (count < 512) {
                // Last ACK.
                ack_current_block(connection);
                connection->state = TFTP_STATE_DONE;
            }

            return count;
        }
    }

    return 0;
}

//--------------------------------------------------------------------------------------------------

void tftp_abort_download(TftpConnection* connection, const char* error_message) {
    send_error(connection, TFTP_ERROR_NOT_DEFINED, error_message);
}
