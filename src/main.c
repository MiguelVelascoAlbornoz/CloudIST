#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/limits.h>
#include "datacenter.h"
#include "parser.h"
#include "datacenter.h"
#include "constants.h"
#include "datacenter_utils.h"

#ifdef _DEBUG //Em modo debug em vez de pasar os argumentos como parametros definem-se logo aqui
	#define SERVERS 1
	#define RAM 17
	#define DISK 350
	#define CPU 16+12
	#define INPUT_DIR "config"
#endif

#define DESIRED_ARGS 6


int main(int argc, char **argv){


	DataCenter dc;
	datacenter_init(&dc);


	size_t servers;
	size_t ram;
	size_t disk;
	double cpu;
	const char* inputDir;
	#ifndef _DEBUG //O executavel debug nao pede argumentos para ser mais facil o debug entao este codigo nao acontece se for debug mode
	if (argc != DESIRED_ARGS) {
		fprintf(stderr, "Usage: %s <servers> <ram> <disk> <cpus> <input dir>\n", argv[0]);
		return 1;
	}
	if (parse_size_t_arg(argv[1], &servers) != 0 ||
			parse_size_t_arg(argv[2], &ram) != 0 ||
			parse_size_t_arg(argv[3], &disk) != 0 ||
			parse_double_arg(argv[4], &cpu) != 0) {
		fprintf(stderr, "Invalid command line arguments.\n");
		return 1;
	}
	inputDir = argv[5];
	#else//Codigo que acontece em debug mode
	printf("Project name: %s\n", PROJECT_NAME);
	printf("Project version: %s\n", PROJECT_VERSION);
	printf("Argumento[0]: %d\nArgv: %s\n",argc,*argv); //Para que não haja warning de variaveis nao utilizadas
		servers = SERVERS;
		ram = RAM;
		disk = DISK;disk = DISK;
		cpu = CPU;
		inputDir = INPUT_DIR;
	#endif


	Resources resources = {
    .ram = ram,
    .disk = disk,
    .cpu = cpu,
	.inputDir = inputDir,
	};

	if(datacenter_configure(&dc, servers, &resources) != 0){
		fprintf(stderr, "Failed to configure Data Center.\n");
		return 1;
	}

	process_fd(STDIN_FILENO, &dc);

	datacenter_destroy(&dc);
}
