#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <linux/limits.h>
#include "datacenter.h"
#include "parser.h"
#include "datacenter.h"
#include "constants.h"
#include "datacenter_utils.h"

#ifdef _DEBUG //Em modo debug em vez de pasar os argumentos como parametros definem-se logo aqui
	#define SERVERS 1
	#define RAM 17000
	#define DISK 40000
	#define CPU 600
	#define FIFO_PATH "/tmp/CloudIST/server"
	#define LOCAL_CORES_AVAILABLES 4
	#define MAX_THREADS 20
#endif

#define DESIRED_ARGS 7


int main(int argc, char **argv){


	DataCenter dc;
	datacenter_init(&dc);


	size_t servers;
	size_t ram;
	size_t disk;
	double cpu;
	const char* fifoPath;
	double localCoresCount;
	size_t maxClients;
	#ifndef _DEBUG //O executavel debug nao pede argumentos para ser mais facil o debug entao este codigo nao acontece se for debug mode
	if (argc != DESIRED_ARGS) {
		fprintf(stderr, "Usage: %s <servers> <ram> <disk> <cpus> <cores-disponiveis-para-CloudIST> <max-clients> <nome_do_FIFO_de_registo>\n", argv[0]);
		return 1;
	}
	if (parse_size_t_arg(argv[1], &servers) != 0 ||
			parse_size_t_arg(argv[2], &ram) != 0 ||
			parse_size_t_arg(argv[3], &disk) != 0 ||
			parse_double_arg(argv[4], &cpu) != 0 ||
			parse_double_arg(argv[5], &localCoresCount) != 0 ||
			parse_size_t_arg(argv[6], &maxClients) != 0) {
		fprintf(stderr, "Invalid command line arguments.\n");
		return 1;
	}
	fifoPath = argv[7];
	#else//Codigo que acontece em debug mode
	printf("Project name: %s\n", PROJECT_NAME);
	printf("Project version: %s\n", PROJECT_VERSION);
	printf("Argumento[0]: %d\nArgv: %s\n",argc,*argv); //Para que não haja warning de variaveis nao utilizadas
		servers = SERVERS;
		ram = RAM;
		disk = DISK;
		cpu = CPU;
		fifoPath = FIFO_PATH;
		localCoresCount = LOCAL_CORES_AVAILABLES;
		maxClients = MAX_THREADS;
	#endif


	Resources resources = {
    .ram = ram,
    .disk = disk,
    .cpu = cpu,
	};

	dc.maxThreads = maxClients;
	dc.availableThreads = maxClients;
	dc.localCoresCount = localCoresCount;
	dc.fifoPath = fifoPath;
	if(datacenter_configure(&dc, servers, &resources) != 0){
		fprintf(stderr, "Failed to configure Data Center.\n");
		return 1;
	}

	waitForConnection();
	//process_fd(STDIN_FILENO, &dc);

	//Sistema para mostrar que o thread nao esta "morto"
	struct timespec deadline;
	clock_gettime(CLOCK_REALTIME, &deadline);  // deadline = ahora
	deadline.tv_sec += 2;
	pthread_mutex_lock(&dc.lock);
	int rc = 0;
	int dotsCount = 1;
	while (dc.running_vms > 0) { //Basicamente esperar pelos processos filhos terminarem
		rc = pthread_cond_timedwait(&dc.cond, &dc.lock, &deadline);
		//O thread que espera pelos processos filhos terminarem acorda ao thread cada vez que um processo filho termine
		//Assim esta parte de codigo fica a comrpobar se ja todos os processos filhos terminaram cada vez que algum deles termina.

		if (rc == ETIMEDOUT) {
			static const char *frames[] = {".", "..", "..."};

			printf("\rWaiting for %ld vms to finish %s       ",dc.running_vms, frames[dotsCount % 3]);
			fflush(stdout);
			dotsCount++;

			clock_gettime(CLOCK_REALTIME, &deadline);
			deadline.tv_sec += 1;

		}
	}
	fflush(stdout);
	printf("\n");
	dc.shutting_down = 1;
	//Uma vez todos os filhos terem terminado o thread de espera pelos processos filhos ficara dormido novamente (por isso deve ser acordado).
	//Como dc.shuttin_down = 1 esse thread entrar numa condicional que o leva ao seu fim
	pthread_cond_broadcast(&dc.cond);

	pthread_mutex_unlock(&dc.lock);
	pthread_join(dc.waitThread, NULL);

	datacenter_destroy(&dc);
}
