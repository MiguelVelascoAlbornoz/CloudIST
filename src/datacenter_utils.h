#ifndef DATACENTER_UTILS__H
#define DATACENTER_UTILS__H

#include "datacenter.h"
#include "utils/vector.h"
#include <pthread.h>



/**
 * Checks if a VMType with the given id exists.
 * 
 * @param dc Pointer to Data Center.
 * @param type_id Id of VM type being checked.
 * 
 * @return Pointer to VMType if it exists.
 * @return NULL if VMType does not exist.
 */
VMType *VMType_exists(DataCenter *dc, const char* type_id);

/**
 * Simulates the placement of every VM requested in the reservation using a
 * temporary copy of each server's available resources. The Data Center state
 * is left unchanged regardless of the outcome.
 *
 * Validation fails if any requested server does not have enough available
 * resources to host all the requested VMs assigned to it or if the request 
 * does not have valid parameters (VM types that do not exist, invalid server ids).
 *
 * @param dc Data Center where the reservation will be validated.
 * @param reservation Reservation to validate.
 *
 * @return 0 if the reservation can be fully allocated.
 * @return 1 otherwise.
 */
int reservation_validate(DataCenter *dc, Reservation *reservation);

/**
 * Creates all VMs requested by the reservation, assigns them to their
 * designated servers, updates the server resource availability, and stores
 * references to the created VMs in both the reservation and the corresponding
 * server hosted VM list.
 *
 * The reservation is expected to have already passed validation before this
 * function is called. If an error occurs during the commit process, the
 * function performs a rollback of the changes made by this operation.
 *
 * @param dc Data Center where the reservation will be committed.
 * @param reservation Reservation to allocate.
 *
 * @return 0 if the reservation was successfully committed
 * @return 1 if an error occurred and the changes were rolled back.
 */
int reservation_commit(DataCenter *dc, Reservation *reservation);

/**
 * Looks up a reservation by id and checks that it is pending.
 *
 * @param dc Pointer to a Data Center.
 * @param reservation_id ID of the reservation to look up.
 *
 * @return Pointer to the Reservation if found and RES_STATE_PENDING.
 * @return NULL if not found or not in RES_STATE_PENDING (error is printed).
 */
Reservation *find_pending_reservation(DataCenter *dc, const char *reservation_id);

/**
 * Frees every VM allocated to the reservation, returns their resources to the
 * corresponding hosting servers, removes them from each server's hosted VM
 * list, and finally removes the reservation from the Data Center reservation
 * list.
 *
 * @param dc Pointer to the Data Center containing the reservation.
 * @param reservation Pointer to the reservation to destroy.
 */
void reservation_destroy(DataCenter *dc, Reservation *reservation);

/**
 *  @details Esta função apenas deve ser chamada por um processo filho!!
 * Runs in the child process after fork(): sets resource limits (RAM,
 * disk) and execs the VM's workload under cpulimit. Never returns
 * (either execs successfully or calls exit(1) on failure).
 *
 * @param vm Pointer to the VM being spawned.
 */
void spawn_vm_child(VM *vm,double localCoresCount, size_t numServers);

/**
 * @brief Esta função deve ser chamada sempre que seja necessario criar um novo thread para o sistema pois sem ela seria impossivel tomar registo dos threads existentes. Ela automaticamente garante que a criação de um thread resserve um espaço e a sua destruição o liberte
 * Esta função apenas pode ser chamada pelo thread principal e mais ninguem sffv
 * @param dc em que sera criado o thread
 * @param func função do thread
 * @param tid variavel em que sera armazenada o tid o datacenter criado
 * @return
 * Retorna se -2 se falhou a criação do thread
 * Retorna se -1 se o datacenter nao tem mais thread disponiveis
 * Retorna 1 se o thread foi criado corretamente
 */
int askDataCenterThread(DataCenter* dc,pthread_t* tid,void *(*func) (void *),void* args);
/**
 * Forks one child per VM in the reservation, calling spawn_vm_child()
 * in each child and recording the pid/state in the parent.
 *
 * @param res Pointer to the Reservation whose VMs should be spawned.
 *
 * @return 0 on success.
 * @return 1 if directories creation fails.
 */
int spawn_all_vms(Reservation *res, double localCoresCount, size_t numServers);

/**
 * Blocks until every VM's process has exited, updating each VM's state
 * to VM_STATE_TERMINATED as its pid is reaped.
 *
 * @param res Pointer to the Reservation whose VMs should be waited on.
 */
void wait_for_all_vms(Reservation *res);

/**
 * Converts a VM state to a human-readable string. Used by datacenter_list().
 *
 * @param state The VMState to convert.
 *
 * @return Static, human-readable string describing the state.
 */
const char *vm_state_to_string(VMState state);

/**
 * Converts a reservation state to a human-readable string. Used by
 * datacenter_list().
 *
 * @param state The ReservationState to convert.
 *
 * @return Static, human-readable string describing the state.
 */
const char *res_state_to_string(ReservationState state);

/**
 * @return
 * 1 se o fileName acabar com .conf e 0 caso contrario
 */
int ends_with_conf(const char *filename);
/**
 *  @brief
 *  Função auxiliiar para usar no qsort
 *  @return
 *  Retorna a mesma coisa que strcmp
 */
int comparar_strings(const void *a, const void *b);

typedef struct {
    Vector* configFiles;
} InsertVectorContext;

/**
 * @brief Callback chamado por cada ficheiro obtido de um diretorio que insere o nome desse ficheiro num vetor segundo uns filtros.
 * Não certifica que entryName seja um ficheiro, ainda poderia ser um diretorio
 * @param entryName -> nome do ficheiro obtido
 * @param context -> Contem toda a informação necessaria para inserir o nome no vetor
 * @return
 */
int insertEntryInVectorCallback(const char* entryName, void* context);
/**
 * @brief
 * Do file descriptor dado lé a proxcima linha e executa o comando ao qual essa linha corresponde
 * @param fd fd a ser lido
 * @param dc  datacenter no qual serão aplicados os comandos
 * @return
 * ->Retorna 1 se o fd chegou ao fim
 * ->Retorna 0 se ainda há mais para ler
 */
int process_command(int fd, DataCenter *dc);
/**
 * @brief
 * Para um file descriptor lé todas as linhas ate chegar ao final.
 * @param fd file descriptor a ser lido
 * @param dc DataCenter a ser aplicados os comandos
 */
void process_fd(int fd, DataCenter *dc);
/**
 * @details out nao deve estar inicializado antes da chamada
 * @brief Devolve um vetor ordenado com os nomes dos vetores .conf no diretorio dado
 * @details O usuario deve chamar porsteriormente destroyVector
 * @return
 *  -> 0 em caso de erro
 */
int list_config_files(const char *dir, Vector *out);

//Esta função ira copiar recursivamente para cada reservação e cada VM da sua respetiva reservação os ficheiros da diretoria de entrada do propio VMType a uma diretoria tal que
// -> /tmp/CloudIST/<ID-reserva>/<ID-VM>
/**
 * @details Esta função nao altera nada do datacenter apenas le as reservas para realizar a copia recursiva.
 * Se a copia falhar o diretorio nao existira
 * @param pointer da vm cujo diretorio deve ser criado
 * @return
 * 0 se aconteceu algum problema
 * 1 se aconteceu todo certo
 */
int initializeVmDirectories(VM* vmToInitialize, const char* reservationID);
/**
 *@details Em caso de falhar nao existiram diretorios nenhums
 * @brief para cada nome em configFiles vai construir o path inteiro com inputDir verificar que existe como ficheiro e obter o seu fileDescriptor. Uma vez com o file descriptor vai chamar process_fd para executar os comandos do ficheiro
 * @param dc datacenter no qual serao executados os config
 * @param configFiles Vector que contem os nomes dos ficheiros config na pasta com path inputDir
 * @param inputDir path do diretorio em que estao os ficheiros config
 */
void datacenterLoadConfigFiles(DataCenter* dc, Vector* configFiles, const char *inputDir);
#endif // DATACENTER_UTILS__H