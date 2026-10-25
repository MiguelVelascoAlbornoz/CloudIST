#include "datacenter_utils.h"

#include <errno.h>
#include <fcntl.h>

#include "datacenter.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/limits.h>
#include <sys/wait.h>
#include <sys/resource.h>

#include "parser.h"
#include "utils/vector.h"

VMType *VMType_exists(DataCenter *dc, const char* type_id){
	for(size_t i = 0; i < dc->num_vm_types; i++){
		if(strncmp(type_id, dc->vm_types[i].id, MAX_STRING_SIZE) == 0){
			return &dc->vm_types[i];
		}
	}
	return NULL;
}

/**
 * Checks if a reservation with the given id already exists in the Data Center.
 * 
 * @param dc Pointer to Data Center.
 * @param reservation_id Id of the reservation being checked.
 * 
 * @return 1 if the reservation exists.
 * @return 0 if the reservation does not exist.
 */	
static int reservation_exists(DataCenter *dc, const char* reservation_id){
	for(size_t i = 0; i < dc->num_reservations; i++){
		if(strncmp(reservation_id, dc->reservations[i]->id, MAX_STRING_SIZE) == 0){
			return 1;
		}
	}
	return 0;
}

int reservation_validate(DataCenter *dc, Reservation *reservation) {
	if(reservation_exists(dc, reservation->id)){
		fprintf(stderr, "Reservation with id %s already exists.\n", reservation->id);
		return 1;
	}

	Resources available[dc->num_servers];
	size_t hosted_count[dc->num_servers];
	size_t vms_count = 0;

	/* Copy current server resources and hosted_vms so we can simulate the allocation. */
	for (size_t i = 0; i < dc->num_servers; i++) {
		available[i] = dc->servers[i].available;
		hosted_count[i] = dc->servers[i].num_hosted_vms;
	}

	for (size_t i = 0; i < reservation->items_count; i++) {
		ReservationItem *item = &reservation->items[i];

		if (item->amount == 0) {
			fprintf(stderr, "Reservation item must request at least one VM.\n");
			return 1;
		}

		VMType *vm_type = VMType_exists(dc, item->vm_type_id);

		if(vm_type == NULL){
			fprintf(stderr, "VM of type %s does not exist\n", item->vm_type_id);
			return 1;
		}

		if (item->server_id == 0 || item->server_id > dc->num_servers) {
			fprintf(stderr, "Invalid server in reservation.\n");
			return 1;
		}

		for (size_t j = 0; j < item->amount; j++) {
			if(vms_count >= MAX_RESERVATION_VMS){
				fprintf(stderr, "Reached max of VMS for a reservation.\n");
				return 1;
			}
			
			if (hosted_count[item->server_id - 1] >= MAX_HOSTED_VMS) {
        fprintf(stderr,
                "Server \"%zu\" reached the maximum number of hosted VMs.\n",
                item->server_id);
        return 1;
    	}

			if (!resources_can_fit(available[item->server_id-1], vm_type->required)) {
				fprintf(stderr, "Server \"%zu\" does not have enough resources.\n", item->server_id);
				return 1;
			}

			resources_sub(&available[item->server_id-1], vm_type->required);
			hosted_count[item->server_id-1]++;
			vms_count++; // One VM "reserved"
		}

		// Attach vm_type to item for later use.
		item->vm_type = vm_type;
	}

	return 0;
}

/**
 * Rolls back the changes made by a reservation operation.
 * 
 * @param reservation Pointer to the reservation being rolled back.
 * @param initial_vms Number of VMs that existed before the operation.
 */
static void reservation_rollback(Reservation *reservation, size_t initial_vms){
	while (reservation->num_vms > initial_vms) {
		VM *vm = reservation->vms[reservation->num_vms - 1];

		Server *server = vm->server;

		// Restore resources
		resources_add(&server->available, vm->type->required);

		// Remove VM from server hosted list
		server->num_hosted_vms--;

		server->hosted_vms[server->num_hosted_vms] = NULL;

		// Remove VM from reservation
		reservation->num_vms--;

		reservation->vms[reservation->num_vms] = NULL;

		free(vm);
	}
}
int ends_with_conf(const char *filename){
	size_t len = strlen(filename);
	if(len < 5) return 0;
	return strcmp(filename + len - 5,".conf") == 0;
}
int reservation_commit(DataCenter *dc, Reservation *reservation){
	// Keep track of how many vms existed before.
	size_t initial_vms = reservation->num_vms;

	for (size_t i = 0; i < reservation->items_count; i++) {
		ReservationItem *item = &reservation->items[i];
		Server *server = &dc->servers[item->server_id - 1];

		for (size_t j = 0; j < item->amount; j++) {
			VM *new_vm = calloc(1, sizeof(VM));
			if (new_vm == NULL) {
				fprintf(stderr, "Failed to allocate VM.\n");
				reservation_rollback(reservation, initial_vms);
				return 1;
			}

			snprintf(new_vm->id,
				MAX_VM_ID_STRING,
				"%s_%zu",
				reservation->id,
				reservation->num_vms + 1
			);

			new_vm->type = item->vm_type;
			new_vm->server = server;
			new_vm->state = VM_STATE_RESERVED;

			resources_sub(&server->available, item->vm_type->required);

			server->hosted_vms[server->num_hosted_vms++] = new_vm;
			reservation->vms[reservation->num_vms++] = new_vm;
		}
	}

	reservation->state = RES_STATE_PENDING;

	return 0;
}

Reservation *find_pending_reservation(DataCenter *dc, const char *reservation_id) {
	Reservation *res = NULL;

	for (size_t i = 0; i < dc->num_reservations; i++) {
		if (strcmp(dc->reservations[i]->id, reservation_id) == 0) {
			res = dc->reservations[i];
			break;
		}
	}

	if (!res) {
		fprintf(stderr, "Reservation ID %s not found.\n", reservation_id);
		return NULL;
	}

	if (res->state != RES_STATE_PENDING) {
		fprintf(stderr, "Reservation ID %s is not in a pending state.\n", reservation_id);
		return NULL;
	}

	return res;
}

void vm_release(VM *vm) {
	Server *server = vm->server;
	for (size_t j = 0; j < server->num_hosted_vms; j++) {
		if (server->hosted_vms[j] == vm) {
			// Solo se devuelven recursos si la VM seguía en el servidor.
			resources_add(&server->available, vm->type->required);
			memmove(&server->hosted_vms[j],
					&server->hosted_vms[j + 1],
					(server->num_hosted_vms - j - 1) * sizeof(VM *));
			server->num_hosted_vms--;
			server->hosted_vms[server->num_hosted_vms] = NULL;
			return;
		}
	}
}
void reservation_destroy(DataCenter *dc, Reservation *reservation) {
	// Free every VM belonging to the reservation.
	char resPath[MAX_PATH_SIZE];
	int n = snprintf(resPath, sizeof(resPath), "%s%s", TMP_FILES_PATH, reservation->id);
	if (n > 0 && (size_t)n < sizeof(resPath) && path_exists(resPath)) {
		if (!removeDir(resPath))
			fprintf(stderr, "Failed to remove directory \"%s\".\n", resPath);
	}
	for (size_t i = 0; i < reservation->num_vms; i++) {
		VM *vm = reservation->vms[i];

		vm_release(vm);
		free(vm);
		reservation->vms[i] = NULL;
	}

	reservation->num_vms = 0;

	// Find the reservation inside the Data Center reservation list.
	size_t idx;
	for (idx = 0; idx < dc->num_reservations; idx++) {
			if (dc->reservations[idx] == reservation)
			{
				break;
			}
	}

	// Reservation not found (should never happen).
	if (idx == dc->num_reservations)
		return;

	// Remove the reservation by shifting the remaining ones.
	memmove(&dc->reservations[idx],
		&dc->reservations[idx + 1],
		(dc->num_reservations - idx - 1) * sizeof(Reservation *));
	dc->num_reservations--;
	dc->reservations[dc->num_reservations] = NULL;
	free(reservation);
}

void spawn_vm_child(VM *vm, const char* executionPath,const char* execDir,const char* limit) {
	(void)vm;

	struct rlimit lim;
	if (chdir(executionPath) != 0) _exit(126);
	lim.rlim_cur = lim.rlim_max = (rlim_t)vm->type->required.ram << 30;
	if (setrlimit(RLIMIT_AS, &lim)) _exit(126);

	lim.rlim_cur = lim.rlim_max = (rlim_t)vm->type->required.disk << 30;
	if (setrlimit(RLIMIT_FSIZE, &lim)) _exit(126);

	if (access(execDir, X_OK) != 0) { //Estavam a haver erros de permission denied e o debug estava a ser muito dificil
		perror(execDir);
		_exit(126);
	}

	execlp("cpulimit", "cpulimit", "-f","-q", "-l", limit,
		   "--", execDir, (char *)NULL);
	printf("Exec falhado");
}

size_t spawn_all_vms(Reservation *res,double localCoresCount, size_t numServers,DataCenter* dc) {
	size_t count = 0;


	//Garantimos a leitura segura das vms durante a copia recursiva
	VM *vms[MAX_RESERVATION_VMS];
	int copied[MAX_RESERVATION_VMS];
	size_t n = res->num_vms;
	memcpy(vms, res->vms, n * sizeof(VM *));

	res->state = RES_STATE_RUNNING;
	res->running_vms++;          //Garantimos que a resserva nao pode ser destruida

	pthread_mutex_unlock(&dc->lock); //Se faz a copia recursiva sem lock
	for (size_t i = 0; i < n; i++)
	{
		copied[i] = initializeVmDirectories(vms[i], res->id);
	}
	pthread_mutex_lock(&dc->lock);


	for (size_t i = 0; i < n; i++) {
		VM *vm = vms[i];
		if (!copied[i]) {
			vm->state = VM_STATE_INIT_FAILED;
			vm_release(vm);
			fprintf(stderr, "Failed to initialize directories of VM: \"%s\" .\n",vm->id);
			continue;
		}

		char execAbs[MAX_PATH_SIZE];
		if (absolute_path(vm->path, execAbs, sizeof(execAbs)) != 0) {
			vm->state = VM_STATE_INIT_FAILED;
			vm_release(vm);
			fprintf(stderr, "Cannot resolve execution path of VM %s.\n", vm->id);
			continue;
		}
		char execDir[MAX_PATH_SIZE];
		if (absolute_path(vm->type->exec_path, execDir, sizeof(execDir)) != 0) {
			vm->state = VM_STATE_INIT_FAILED;
			vm_release(vm);
			fprintf(stderr, "Cannot resolve executable path of VM %s.\n", vm->id);
			continue;
		}
		char limit[32]; //Transforma
		//atualmente todos os servers tem o mesmo cpu
		int CPULIMIT = (int)((double)100.0f*vm->type->required.cpu *localCoresCount / (vm->server->total.cpu*(double)numServers));
		if (CPULIMIT < 1)
		{
			CPULIMIT = 1;
		} else if (CPULIMIT > 1200)
		{
			CPULIMIT = 1200;
		}
		snprintf(limit, sizeof(limit), "%d", (int)(CPULIMIT));

		pid_t pid = fork();

		if (pid < 0)
		{
#ifdef _DEBUG
			fprintf(stderr,"FORK FALHOU");
#endif
		} else if (pid == 0)
		{

			spawn_vm_child(vm,execAbs,execDir,limit);
#ifdef _DEBUG
			dprintf(STDERR_FILENO, "[child %s] exec %s limit=%s\n", vm->id, execDir, limit);
#endif

			_exit(127);
		} else
		{
			count++;
			vm->state = VM_STATE_RUNNING;
			vm->pid = pid;
			dc->running_vms++;
			res->running_vms++;
			pthread_cond_broadcast(&dc->cond); //Thread de espera deve ser acordado imediato
			#ifdef _DEBUG
			printf("VM with id %s running.\n",vm->id);
			#endif
		}

	}
	res->running_vms--;

	return count;
}


const char *vm_state_to_string(VMState state) {
    switch (state) {
        case VM_STATE_RESERVED:   return "RESERVED";
        case VM_STATE_RUNNING:    return "RUNNING";
        case VM_STATE_TERMINATED: return "TERMINATED";
    	case VM_STATE_INIT_FAILED: return "INIT_FAILED";
        default:                  return "UNKNOWN";
    }
}

const char *res_state_to_string(ReservationState state) {
    switch (state) {
			case RES_STATE_PENDING:  return "PENDING";
			case RES_STATE_RUNNING:  return "RUNNING";
			case RES_STATE_FINISHED: return "FINISHED";
			default:                 return "UNKNOWN";
    }
}

int comparar_strings(const void *a, const void *b) {
	const char* str_a = (const char *)a;
	const char* str_b = (const char *)b;
	return strcmp(str_a, str_b);
}
int insertEntryInVectorCallback(const char* entryName, void* context) {
	InsertVectorContext* insertContext = context;
	if (ends_with_conf(entryName)) { //Certificar que é um ficheiro e que termina em .conf
		//Necessario fazer isto por que de outra forma iriamos ter segfault
		char name[NAME_MAX + 1] = {0};
		snprintf(name, sizeof(name), "%s", entryName);
		if (!push_back(insertContext->configFiles,(void*)name)) {
			fprintf(stderr, "Error creating config files vector in file: %s\n", name);
			return 0;
		}
	}
	return 1;
}

int process_command(int fd, DataCenter *dc)
{

	switch (get_next_command(fd)){

			case CMD_DEFINE: {

				VMType vmtype;

				if (parse_define(fd, &vmtype) != 0) {
					fprintf(stderr, "Invalid define command. See H (help) for usage.\n");
					break;
				}
				pthread_mutex_lock(&dc->lock);
				if(datacenter_define_VM(dc, &vmtype) != 0){
					fprintf(stderr, "Failed to define VM.\n");
					pthread_mutex_unlock(&dc->lock);
					break;
				}
				pthread_mutex_unlock(&dc->lock);
				printf("VM successfully defined!\n");

				break;

			}

			case CMD_RESERVE: {
				Reservation reservation = {0};

				size_t num_items = parse_reserve(fd, &reservation, MAX_RESERVATIONS_ITEMS);

				if (num_items == 0) {
					fprintf(stderr, "Invalid reserve command. See H (help) for usage.\n");
					break;
				}
				pthread_mutex_lock(&dc->lock);
				if (datacenter_reserve(dc, &reservation) != 0) {
					fprintf(stderr, "Failed to reserve VMs.\n");
					pthread_mutex_unlock(&dc->lock);
					break;
				}
				pthread_mutex_unlock(&dc->lock);
				printf("Reservation made successfully!\n");

				break;
			}

			case CMD_EXECUTE:
				char id[MAX_STRING_SIZE];

				if(parse_execute(fd, id) != 0){
					fprintf(stderr, "Invalid execute command. See H (help) for usage.\n");
					break;
				}
			pthread_mutex_lock(&dc->lock);
				if (datacenter_execute(dc, id) != 0) {
					fprintf(stderr, "Failed to execute reservation.\n");
					pthread_mutex_unlock(&dc->lock);
					break;
				}
			pthread_mutex_unlock(&dc->lock);


				break;

			case CMD_LIST:
			pthread_mutex_lock(&dc->lock);
				if (datacenter_list(dc) != 0) {
					fprintf(stderr, "Failed to list VMs.\n");
				}
			pthread_mutex_unlock(&dc->lock);
				break;

			case CMD_WAIT:
				unsigned int delay;

				if(parse_wait(fd, &delay) != 0){
					fprintf(stderr, "Invalid wait command. See H (help) for usage.\n");
					break;
				}

				datacenter_wait(delay);
				break;

			case CMD_INVALID:
				fprintf(stderr, "Invalid Command. See H (help) for usage.\n");
				break;

			case CMD_HELP:
				printf(
					"Spaces between arguments are allowed, but not after command end.\n"
					"Available commands:\n"
					" D <VM_TYPE_ID> <INPUT_FOLDER> <EXECUTABLE_PATH> <RAM_NEEDED> <DISK_NEEDED> <VCPU_NEEDED_COUNT>\n"
					" R <RESERVATION_ID> [<VM_TYPE_ID> <COUNT> <SERVER_ID>]+\n"
					" A <RESERVATION_ID>\n"
					" L\n"
					" E <DELAY_MS>\n"
					" H\n"
				);
				break;

			case CMD_EMPTY:
				break;

			case EOC:

				return 1;
		}
	return 0;
}
void process_fd(int fd, DataCenter *dc) {
	while (!process_command(fd, dc)) { }
}

int list_config_files(const char *dir, Vector *out) {
	if (!initVector(out, NAME_MAX + 1)) return 0;
	InsertVectorContext ctx = { .configFiles = out };
	if (!executePerEachEntry(dir, insertEntryInVectorCallback, &ctx)) {
		destroyVector(out);
		return 0;
	}
	qsort(out->data, out->size, out->dataSize, comparar_strings);
	return 1;
}

int initializeVmDirectories(VM *vmInReservation, const char* reservationID)
{

	char* path = vmInReservation->path;
	int n = snprintf(path, MAX_PATH_SIZE,  "%s%s/%s", TMP_FILES_PATH,reservationID, vmInReservation->id);
	if (n < 0 || (size_t)n >= MAX_PATH_SIZE) {
		//PATH a junção forma um nome demasiado longo
		fprintf(stderr, "Nomes das resservas demasiado longos como para criar um path.\n");
		*path = '\0';
		return 0;
	}
	//Pode -se fazer agora  acopia recursiva
	if (!copiaRecursiva(vmInReservation->type->input_folder,path)) {
		return 0;
	}


	return 1;
}
void datacenterLoadConfigFiles(DataCenter* dc, Vector* configFiles, const char *inputDir)
{
	size_t n = configFiles->size;
	if (n == 0) return;

	pthread_t *tids = malloc(n * sizeof(*tids)); //Lista de tids para esperar apos ter mandado ler os config files
	ConfigLoaderArgs *args = malloc(n * sizeof(*args)); //Argumentos de cada thread criado
	int *started = calloc(n, sizeof(*started));
	if (!tids || !args || !started) {
		fprintf(stderr, "Sem memoria para poder carregar os config files.\n");
		free(tids); free(args); free(started);
		return;
	}

	for (size_t i = 0; i < n; i++) {
		args[i].dc = dc;
		args[i].inputDir = inputDir;
		args[i].name = (char*)getAtVector(configFiles, i);

		if (askDataCenterThread(dc, &tids[i], configLoaderThread, &args[i]) > 0) {
			started[i] = 1;
		} else {
			configLoaderThread(&args[i]); //Quando por algum motivo não é possivel chamar a criação de um thread o thread principal ajuda na leitura dos ficheiros
		}
	}
	//Esperar cada thread
	for (size_t i = 0; i < n; i++)
	{
		if (started[i])
		{
			pthread_join(tids[i], NULL);
		}
	}


	free(tids); free(args); free(started);

}
typedef struct {
	DataCenter *dc;
	void*(*func) (void*);
	void *arg;
} ThreadCreationArgs;
/**
 * @brief Simplesmente liberta um thread antes da sua morte. Sempre deve ser executado que um thread do sistema termine
 * @param p Deve ser um datacenter
 */
static void releaseSlot(void *p) {
	DataCenter *dc = p;
	pthread_mutex_lock(&dc->threadCreationMutex);
	dc->availableThreads++;
	pthread_mutex_unlock(&dc->threadCreationMutex);
}
/**
 * @brief Esta função "substitue" a criação de um thread pois é ela quem realmente chama á função dada para o thread
 * Mas entre tanto garante a libertação de thread no sistema antes da sua destruição. É como uma função que simplifica a notificação do fim de um thread ao datacenter.
 * @param p informação nevcessaria para criar um thread e notifica-lo no sistema.
 */
static void* threadWrapper(void *p) {
	ThreadCreationArgs a = *(ThreadCreationArgs *)p;//Copia so o conteudo
	free(p);


	void *r = NULL;
	pthread_cleanup_push(releaseSlot, a.dc);
	r = a.func(a.arg);
	pthread_cleanup_pop(1);
	return r;
}
int askDataCenterThread(DataCenter* dc,pthread_t* tid,void *(*func) (void *),void* args) {
	pthread_mutex_lock(&dc->threadCreationMutex);
	if (dc->availableThreads <= 0) {
		pthread_mutex_unlock(&dc->threadCreationMutex);
		return -1;
	}


	ThreadCreationArgs* a = malloc(sizeof(ThreadCreationArgs));
	if (!a)
	{
		pthread_mutex_unlock(&dc->threadCreationMutex);
		return -2;
	}
	dc->availableThreads--;//Se faz a resserva logo
	pthread_mutex_unlock(&dc->threadCreationMutex);
	a->dc = dc;
	a->func = func;
	a->arg = args;
	if (pthread_create(tid,NULL,threadWrapper,a) != 0) {
		free(a);
		pthread_mutex_lock(&dc->threadCreationMutex);
		dc->availableThreads++;//Se faz a resserva logo
		pthread_mutex_unlock(&dc->threadCreationMutex);
		return -2;
	}
	return 1;
}
void* waitingThread(void* arg) {
	DataCenter *dc = arg;
	while (1) { //Unicamente para quando o datacenter finalizar
		//Dormir o thread
		pthread_mutex_lock(&dc->lock);
		while (dc->running_vms == 0 && !dc->shutting_down) pthread_cond_wait(&dc->cond, &dc->lock); //Unicamente vai ser acordado durante a criação de um processo de uma VM OU na finalização do thread principal

		//Caso em que o thread principal anuncia o fim da execução
		if (dc->running_vms == 0 && dc->shutting_down) {
			pthread_mutex_unlock(&dc->lock);
			return NULL;
		}
		pthread_mutex_unlock(&dc->lock);
		//esta parte do cdigo apenas acontece se existe pelo menos 1 vm em execução pelo que ficamos em modo espera de esse processo
		int status;
		pid_t pid = wait(&status);
		//Existe uma condição pela qual este wait pode parar sem nenhuma VM ter terminado o seu processo
		//	Quando existe uma chamada a removeDir este wait poderia chegar a acordar no fim dessa chamada
		//	De momento o unico problmea que isso pode causar é um falso sinal de erro na propia chamada de removeDir
		if (pid < 0) {
			struct timespec ts = { .tv_sec = 0, .tv_nsec = 1000000 }; // 1 ms
			nanosleep(&ts, NULL);
			continue;
		}
		//Temos de garantir que as reservações nao estão a mudar
		pthread_mutex_lock(&dc->lock);
		//Obter a vm e a reservação
		Reservation* reservation;
		VM* vm;
		getDatacenterInfoFromPID(dc,pid,&reservation,&vm);
		if (vm ==NULL || reservation == NULL) {
			//Neste caso wait recebeu o fim de um processo que nao corresponde ao de nenhuma VM pelo que simplesmente volta-se ao inicio do while
			pthread_mutex_unlock(&dc->lock);
			continue;
		}
		if (WIFSIGNALED(status)) {
			fprintf(stderr, "VM %s terminated by signal %d\n", vm->id, WTERMSIG(status));
		} else if (WIFEXITED(status)) {
#ifdef _DEBUG
			fprintf(stderr, "VM %s exited with code %d\n", vm->id, WEXITSTATUS(status));
#else
			if (WEXITSTATUS(status) != 0)
				fprintf(stderr, "VM %s exited with code %d\n", vm->id, WEXITSTATUS(status));
#endif
		}
		vm->state = VM_STATE_TERMINATED;
		vm_release(vm);
		dc->running_vms--;
		reservation->running_vms--;
		if (reservation->running_vms <= 0) {
			reservation->state = RES_STATE_FINISHED;
			reservation_destroy(dc, reservation);   // invalida 'res'; não o uses depois
		}
		pthread_cond_broadcast(&dc->cond); //Notificar ao thread principal que um processo de uma VM terminou
		pthread_mutex_unlock(&dc->lock);
	}
}
void getDatacenterInfoFromPID(DataCenter *dc, pid_t pid,Reservation** reservation, VM** vm) {
	*vm = NULL;
	*reservation = NULL;
	for (size_t i = 0; i < dc->num_reservations; i++) {
		Reservation *res = dc->reservations[i];
		for (size_t j = 0; j < res->num_vms; j++) {
			VM *vmL = res->vms[j];
			if (vmL->pid == pid) {
				*vm =vmL;
				*reservation = res;
			}
		}
	}

}

void* configLoaderThread(void* args)
{
	ConfigLoaderArgs* configArgs = args;
	//Criação do path completo
	char configFilePath[PATH_MAX];
	if (snprintf(configFilePath, sizeof(configFilePath), "%s/%s",configArgs->inputDir, configArgs->name) >= (int)sizeof(configFilePath)) {
		fprintf(stderr, "Path too long in %s\n",    configArgs->inputDir);
		return NULL;
	}
	//Verificar que é um ficheiro e nao um diretorio
	if (!file_exists(configFilePath)) return NULL;
	int configFD = open(configFilePath, O_RDONLY);
	if (configFD < 0) {
		fprintf(stderr, "Error opening file %s: %s\n", configFilePath, strerror(errno));
		return NULL;
	}
	//Uma vez o file descriptor obtido agora é so usra as funções par ler comandos que ja existem
	process_fd(configFD,configArgs->dc);
	close(configFD);
	return NULL;
}