#include <string.h>

#include "../client/ConnectionParser.h"
//
// Created by migue on 24/11/2026.
//

void encodeConnectionRequest(char buffer[CONNECT_REQUEST_SIZE],const char req_pipe_path[FIFO_NAME_MAX_SIZE],const char resp_pipe_path[FIFO_NAME_MAX_SIZE], unsigned int client_id)
{
    memset(buffer, 0, CONNECT_REQUEST_SIZE); //Inicializa com \0
    buffer[0] = CONNECT_OP_CODE;
    buffer += 1;
    memccpy(buffer,req_pipe_path,0,FIFO_NAME_MAX_SIZE);
    buffer += FIFO_NAME_MAX_SIZE;
    memccpy(buffer,resp_pipe_path,0,FIFO_NAME_MAX_SIZE);
    buffer += FIFO_NAME_MAX_SIZE;
    memcpy(buffer,&client_id,sizeof(unsigned int));
}


void decodeConnectionRequest(const char buffer[CONNECT_REQUEST_SIZE],char req_pipe_path[FIFO_NAME_MAX_SIZE],char resp_pipe_path[FIFO_NAME_MAX_SIZE],unsigned int* client_id)
{
    memcpy(req_pipe_path, buffer, FIFO_NAME_MAX_SIZE);
    buffer += FIFO_NAME_MAX_SIZE;
    memcpy(resp_pipe_path, buffer, FIFO_NAME_MAX_SIZE);
    buffer += FIFO_NAME_MAX_SIZE;
    memcpy(client_id, buffer, sizeof(unsigned int));
}
/**
 *
 * @param buffer Deve garantir-se a escrita em CONNEC_RESPONSE_SIZE bytes
 */
void encodeConnectionResponse(char buffer[CONNECT_RESPONSE_SIZE],char result, size_t num_servers, size_t ram, size_t disk, double cpus)
{
    memset(buffer, 0, CONNECT_RESPONSE_SIZE);
    buffer[0] = result;
    buffer += 1;
    memcpy(buffer,&num_servers,sizeof(size_t));
    buffer += sizeof(size_t);
    memcpy(buffer,&ram,sizeof(size_t));
    buffer += sizeof(size_t);
    memcpy(buffer,&disk,sizeof(size_t));
    buffer += sizeof(size_t);
    memcpy(buffer,&cpus,sizeof(double));
}
void decodeConnectionResponse(char buffer[CONNECT_RESPONSE_SIZE], char* result, size_t* num_servers, size_t* ram, size_t* disk, double* cpus)
{
    memcpy(result,buffer,1);
    buffer += 1;
    memcpy(num_servers,buffer,sizeof(size_t));
    buffer += sizeof(size_t);
    memcpy(ram,buffer,sizeof(size_t));
    buffer += sizeof(size_t);
    memcpy(disk,buffer,sizeof(size_t));
    buffer += sizeof(size_t);
    memcpy(cpus,buffer,sizeof(double));
}