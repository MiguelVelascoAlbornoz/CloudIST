#ifndef CONNECTION_PARSER__H
#define CONNECTION_PARSER__H
#include "common.h"

/**
* @brief
 * Mete a data toda necessaria no buffer com a ordem dada: (char) OP_CODE=1 | (char[]) req_pipe_path | (char[]) resp_pipe_path | client_id
 * Garante que a memoria nao utilizada fica inicializada com \0
 */
void encodeConnectionRequest(char buffer[CONNECT_REQUEST_SIZE],const char req_pipe_path[FIFO_NAME_MAX_SIZE],const char resp_pipe_path[FIFO_NAME_MAX_SIZE], unsigned int client_id);

/**
* @brief
 * Deodifica a resposta dada pelos servidor, considera-se que vem no seguinte formato:  (char) result | (size_t) num_servers | (size_t) ram | (size_t) disk | (double) cpus
 */
void decodeConnectionResponse(char buffer[CONNECT_RESPONSE_SIZE], char* result, size_t* num_servers, size_t* ram, size_t* disk, double* cpus);
#endif
