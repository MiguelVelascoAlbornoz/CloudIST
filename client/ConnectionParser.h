#ifndef CONNECTION_PARSER__H
#define CONNECTION_PARSER__H

#define CONNECT_OP_CODE 1
#define CONNECT_REQUEST_SIZE sizeof(char)+FIFO_NAME_MAX_SIZE*2+sizeof(unsigned int)
#define CONNECT_RESPONSE_SIZE sizeof(char)+sizeof(size_t)*3 + sizeof(double)
#define DISCONNECT_OP_CODE 2
#define DEFINE_OP_CODE 3
#define RESERVE_OP_CODE 4
#define RUN_OP_CODE 5
#define LIST_OP_CODE 6
#define FIFO_NAME_MAX_SIZE 40

/**
 * connected = 1 if connected to server, 0 otherwise
 */
typedef struct Client
{
 int req_fd;
 int resp_fd;
 int connected;
 unsigned int id;
} Client;

/**
 * @brief
 * Mete a data toda necessaria no buffer com a ordem dada: (char) OP_CODE=1 | (char[]) req_pipe_path | (char[]) resp_pipe_path | client_id
 * Garante que a memoria nao utilizada fica inicializada com \0
 * @param buffer Deve garantir-se que permite a escrita de CONNECT_REQUEST_SIZE bytes
 * @param req_pipe_path Deve garantir-se que permite a leitura de FIFO_NAME_MAX_SIZE bytes
 * @param resp_pipe_path Deve garantir-se que permite a leitura de FIFO_NAME_MAX_SIZE bytes
 */
void encodeConnectionRequest(char buffer[CONNECT_REQUEST_SIZE],const char req_pipe_path[FIFO_NAME_MAX_SIZE],const char resp_pipe_path[FIFO_NAME_MAX_SIZE], unsigned int client_id);

/**
 * @brief
 * Decodifica a requisicao dada pelo cliente, considera-se que vem no seguinte formato: (char[]) req_pipe_path | (char[]) resp_pipe_path | client_id
 * @param buffer Deve garantir que permite a leitura de CONNECT_REQUEST_SIZE bytes, não deve conter o opcode
 */
void decodeConnectionRequest(const char buffer[CONNECT_REQUEST_SIZE],char req_pipe_path[FIFO_NAME_MAX_SIZE],char resp_pipe_path[FIFO_NAME_MAX_SIZE],unsigned int* client_id)

/**
* @brief
 * Deodifica a resposta dada pelos servidor, considera-se que vem no seguinte formato:  (char) result | (size_t) num_servers | (size_t) ram | (size_t) disk | (double) cpus
 */
void decodeConnectionResponse(char buffer[CONNECT_RESPONSE_SIZE], char* result, size_t* num_servers, size_t* ram, size_t* disk, double* cpus);

/**
 *
 * @param buffer Deve garantir-se a escrita em CONNEC_RESPONSE_SIZE bytes
 */
void encodeConnectionResponse(char buffer[CONNECT_RESPONSE_SIZE],char result, size_t num_servers, size_t ram, size_t disk, double cpus);
#endif
