#define _XOPEN_SOURCE 700

#include "filesystem.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
int path_exists(const char *path){
  struct stat st;

  if (stat(path, &st) != 0)
    return 0;

  return S_ISDIR(st.st_mode);
}

int file_exists(const char *path){
  struct stat st;

  if (stat(path, &st) != 0)
    return 0;

  return S_ISREG(st.st_mode);
}



int absolute_path(const char *path, char *buffer, size_t size){
  char *resolved = realpath(path, NULL);

  if (resolved == NULL)
    return 1;

  if (strlen(resolved) >= size) {
    free(resolved);
    return 1;
  }

  strcpy(buffer, resolved);

  free(resolved);
  return 0;
}

int copiaFicheiro(const char* srcFile, const char* dstFile) {

  //Getting src file file descriptor
  //Para os file descriptors de um ficheiro em modo read only não é necessario verificar que close() funcionou.
  int srcFD = open(srcFile, O_RDONLY);
  if (srcFD < 0) {
    fprintf(stderr, "Error opening file %s: %s\n", srcFile, strerror(errno));
    return 0;
  }

  //Getting dst file descriptor
  //Creat é equivalente a open(path, O_WRONLY|O_CREAT|O_TRUNC, mode), ou seja, se ja existir abre o e apaga o conteudo que ja existia nele, senão cria o ficheiro
  int dstFD = creat(dstFile,S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (dstFD < 0) {
    fprintf(stderr, "Error creating file %s: %s\n", dstFile, strerror(errno));
    close(srcFD);
    return 0;
  }
  int ok = 1;
  static char buffer[FILE_COPY_CHUNK_SIZE]; //static pois estar a inicializar esta memoria por cada chama-da a função é um desperdiço de recursos
  while (ok) {
    //Escrevem se n bytes no buffer e verificar-se se forem 0, acabou se de ler o ficheiro, se der menor a 0 houver erro e senão continua-se com a escrita desses bytes no ficheiro de destino
    int nRead = read(srcFD, buffer, FILE_COPY_CHUNK_SIZE);
    if (nRead == 0) break; // Fim do ficheiro
    if (nRead <0) {
      if (errno == EINTR) continue;
      ok = 0;
      fprintf(stderr, "Error reading file %s: %s\n", srcFile, strerror(errno));
      break;
    }
    //Cria-se um pointer auxiliar, assim write vai tentar escrever uma quantidade de bytes, caso nao conseguir escrever todos
    //o pointer auxiliar aumenta a quantidade de bytes que foram escritos e a quantidade de bytes que faltam por escrever diminui a mesma quantidade
    //assim volta-se a repetir o write ate der erro ou o numero de bytes por escrever for 0
    char *p = buffer;
    while (nRead > 0) {
      ssize_t nWritten = write(dstFD, p, nRead);
      if (nWritten < 0) {
        if (errno == EINTR) continue;
        fprintf(stderr, "Error writing file %s: %s\n", dstFile, strerror(errno));
        ok = 0;
        break;
      }
      p += nWritten;
      nRead -= nWritten;
    }
  }

    close(srcFD);
    if (close(dstFD) < 0) {
      fprintf(stderr, "Error closing file %s: %s\n", dstFile, strerror(errno));
    }
  return ok;
}
typedef struct {
  const char *src;
  const char *dst;
} CopyContext;
int copyEntryCallback(const char* entryName, void* context) {
  char srcPath[PATH_MAX+1]; //Path completo do ficheiro src atualmente a ser iterado
  char dstPath[PATH_MAX+1]; //Path completo do ficheiro dst que deveria existir por este ficheiro src
  CopyContext *copyContext = context;
  const char *src = copyContext->src;
  const char *dst = copyContext->dst;
  //Criam-se esses tais paths
  if (snprintf(srcPath, sizeof(srcPath), "%s/%s", src, entryName) >= (int)sizeof(srcPath) ||
      snprintf(dstPath, sizeof(dstPath), "%s/%s", dst, entryName) >= (int)sizeof(dstPath)) {
    fprintf(stderr, "Path too long in %s\n", src);
    return 0;
  }
  //Verificar se o path atual a ser iterado é uma pasta ou um ficheiro
  if (path_exists(srcPath)) {
    //Se for uma pasta chama novamente a copia recursiva para esse diretorio
    if (!copiaRecursiva(srcPath, dstPath)) {
      return 0;
    }
  } else if (file_exists(srcPath)) {
    //Se for um file é precisso copiar o ficheiro e mete-lo no dst e continuar para o proximo name que dirent fornecera
    if (!copiaFicheiro(srcPath, dstPath)) {
      return 0;
    }
  }
  return 1;
}
int copiaRecursiva(const char* src, const char* dst) {
  //Certificar que dst existe, senão cria-lo
  if (mkdir(dst,  0777) != 0 && errno != EEXIST) {
    fprintf(stderr, "Error creating directory %s: %s\n", dst, strerror(errno));
    return 0;
  }
  //Procurar agora em cada diretorio
  CopyContext ctx = { src, dst };
  return executePerEachEntry(src, copyEntryCallback, &ctx);
}

int executePerEachEntry(const char* path, EntryCallback func, void* context) {
  DIR* direntDIR = opendir(path);
  if (!direntDIR) {
    return 0;
  }
  struct dirent* dir;
  int ok = 1;
  //Itera-se em cada nome de ficheiros/pastas existentes do diretorio src
  while (1) {
    errno = 0;
    dir = readdir(direntDIR);
    if (!dir) {
      if (errno != 0) {
        ok = 0;
      }
      break;
    }
    if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) //Skip  a coisos que aparecem de forma default
      continue;
    if (!func(dir->d_name, context)) {
      ok = 0;
      break;
    }
  }


  closedir(direntDIR);
  if (errno != 0 || !ok) {
    return 0;
  }

  return 1;
}