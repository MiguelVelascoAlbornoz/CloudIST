#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include "ConnectionParser.h"

#include "api.h"
#include "parser.h"


void pretty_print_cloud_specs(size_t num_servers, size_t ram, size_t disk, double cpus) {
    printf("╔══════════════════════════════════════════╗\n");
    printf("║         CloudIST - Connected!            ║\n");
    printf("╠══════════════════════════════════════════╣\n");
    printf("║                                          ║\n");
    printf("║  Servers       : %-24zu║\n", num_servers);
    printf("║  RAM / Server  : %-24zu║\n", ram);
    printf("║  Disk / Server : %-24zu║\n", disk);
    printf("║  CPUs / Server : %-24.2f║\n", cpus);
    printf("║                                          ║\n");
    printf("╚══════════════════════════════════════════╝\n");
    printf("\n");
}

char *convert_vm_state_to_string(VMState state) {
    switch (state) {
        case VM_STATE_RESERVED:   return "RESERVED";
        case VM_STATE_RUNNING:    return "RUNNING";
        case VM_STATE_TERMINATED: return "TERMINATED";
        default:                  return "UNKNOWN";
    }
}

void print_vm(char * vm_id, char * vm_type_id, size_t server_id, char vm_state) {
    printf("VM ID : %s | VM Type: %s | Server ID: %zu | State: %s\n",
                vm_id,
                vm_type_id,
                server_id,
                convert_vm_state_to_string((VMState) vm_state));
}


int createClientFifos(const char* req_pipe_path,const char* resp_pipe_path,Client *client)
{
    //Caso o o processo do cliente tenha acabado sem fazer unlink
    unlink(req_pipe_path);
    unlink(resp_pipe_path);

    if (mkfifo(req_pipe_path, 0777) <0) //0777 -> leitura, escrita e execução
    {
        return 0;
    }

    if (mkfifo(resp_pipe_path, 0777) < 0)
    {
        unlink(req_pipe_path);
        return 0;
    }
    return 1;
}

/**
 * GUarda los file descriptors de los pipes con los fifos ya creados anteriormente
 * @return Se falhar retorna 0
 */
int openClientPipes(const char* req_pipe_path,const char* resp_pipe_path,Client *client)
{
    int req_fd = open(req_pipe_path, O_WRONLY);
    if (req_fd<0)return 0;
    int resp_fd = open(resp_pipe_path, O_RDONLY);
    if (resp_fd<0)
    {
        close(req_fd);
        return 0;
    }
    client->req_fd = req_fd;
    client->resp_fd = resp_fd;
    return 1;
}
/**
 * Garante o close , unlink e os valores devidos ao nao estar conectado no client
 */
void unlinkClientFifos(const char* req_pipe_path,const char* resp_pipe_path)
{
    unlink(req_pipe_path);
    unlink(resp_pipe_path);
}
void closeClientPipes(Client *client)
{
    close(client->req_fd);
    close(client->resp_fd);
    client->req_fd = -1;
    client->resp_fd = -1;
}


int cloudIst_connect(char const* req_pipe_path, char const* resp_pipe_path, char const* server_pipe_path,Client *client) {
    //Assume-se que o pipe para registar clientes do lado do servidor ja existe.
    //Tambem que o server tem algum open no registo á espera de alguem se conectar.
    if (!createClientFifos(req_pipe_path,resp_pipe_path,client)) return 1;

    int server_fd = open(server_pipe_path, O_WRONLY);

    if (server_fd<0) {
        unlinkClientFifos(req_pipe_path,resp_pipe_path);
        return 1;
    }
    //Apos isto o server acorda e fica bloqueado num read.
    char request[CONNECT_REQUEST_SIZE];
    encodeConnectionRequest(request,req_pipe_path,resp_pipe_path,client->id);

    if (write(server_fd,request, sizeof(request)) != CONNECT_REQUEST_SIZE) {
        close(server_fd);//O server deve acordar e gerir o erro (read vai devolver 0)
        unlinkClientFifos(req_pipe_path,resp_pipe_path);
        return 1;
    }

    close(server_fd); // ja nao serve para nada
    //Agora o server acorda, vai processar a mensagem e ficar bloqueado num open aos pipes do cliente
    //Esperamos que tanto o cliente como o server realizem o open
    if (!openClientPipes(req_pipe_path,resp_pipe_path,client)) //Praticamente assume-se que nao falha nem do lado do server nem do  cliente
    {
        unlinkClientFifos(req_pipe_path,resp_pipe_path);
        return 1;
    }
    //Uma vez o server acordar vai criar e mandar a mensagem de resposta
    //Se o server falhar em enviar a repsosta ira fazer close e bytes == 0
    //Ficamos a espera ate o server mandar a resposta
    char response[CONNECT_RESPONSE_SIZE];
    int bytes = read(client->resp_fd,response, CONNECT_RESPONSE_SIZE);
    if (bytes != CONNECT_RESPONSE_SIZE)
    {
        unlinkClientFifos(req_pipe_path,resp_pipe_path);
        closeClientPipes(client);
        return 1;
    }
    char result;
    size_t num_servers, ram, disk;
    double cpus;
    decodeConnectionResponse(response,&result, &num_servers, &ram, &disk, &cpus);
    if (result != 0)
    {
        unlinkClientFifos(req_pipe_path,resp_pipe_path);
        closeClientPipes(client);
        return 1;
    }
    //Aqui o o server ja o server considera o cliente registado e continua a sua vida
    //Pelo que tambem podemos considerarnos conectados
    client->connected = 1;
    //Print server's specifications to stdout
    pretty_print_cloud_specs(num_servers, ram, disk, cpus);
    return 0;
}



int cloudIst_disconnect(void){
    //TODO close pipes
    printf("Implement me pls\n");
    return 0;
}

int cloudIst_define(VMType vm_type) {
    (void) vm_type;
    //TODO: send define command to the server (through the request pipe) and wait for the response (through the response pipe)
    printf("Implement me pls\n");
    return 0;
}

int cloudIst_reserve(Reservation reservation) {
    (void) reservation;
    //TODO: send reserve command to the server (through the request pipe) and wait for the response (through the response pipe)
    printf("Implement me pls\n");
    return 0;
}

int cloudIst_run(char *reservation_id) {
    (void) reservation_id;
    //TODO: send run/arranca command to the server (through the request pipe) and wait for the response (through the response pipe)
    printf("Implement me pls\n");
    return 0;
}

int cloudIst_list_vms() {
    //TODO: send List command to the server (through the request pipe) and wait for the response (through the response pipe)
    printf("Implement me pls\n");

    //TODO: dont forget to print output to stdout using the format from function print_vm()
    return 0;
}


int send_command(ParsedCommand *cmd) {

    switch (cmd->type) {
        case CMD_DEFINE:
            if (cloudIst_define(cmd->data.vmtype)) {
                fprintf(stderr, "Error defining VM type %s\n", cmd->data.vmtype.id);
            } else {
                fprintf(stdout, "Defined VM %s\n", cmd->data.vmtype.id);
            }
            break;

        case CMD_RESERVE:
            if (cloudIst_reserve(cmd->data.reservation)) {
                fprintf(stderr, "Error making reservation %s\n", cmd->data.reservation.id);
            } else {
                fprintf(stdout, "Reserved %s\n", cmd->data.reservation.id);
            }
            break;

        case CMD_EXECUTE:
            if (cloudIst_run(cmd->data.id)) {
                fprintf(stderr, "Failed to execute reservation %s.\n", cmd->data.id);
            } else {
                printf("Running reservation %s\n", cmd->data.id);
            }
            break;

        case CMD_LIST:
            if (cloudIst_list_vms()) {
                fprintf(stderr, "Failed to list VMs.\n");
            } else {
                printf("show list");
            }
            break;

        case CMD_WAIT:
            if (cmd->data.delay > 0) {
                printf("Waiting...\n");
                struct timespec delay = {
                    .tv_sec = (time_t)(cmd->data.delay / 1000U),
                    .tv_nsec = (long)(cmd->data.delay % 1000U) * 1000000L
                };
                nanosleep(&delay, NULL);
            }
            break;
            
        case CMD_EMPTY:
            break;

        case CMD_HELP: 
            break;

        case CMD_INVALID:
            return 1;

        case EOC:
            return 1;

    }

    return 0;
}