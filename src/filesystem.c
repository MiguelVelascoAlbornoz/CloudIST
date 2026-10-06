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


int copiaRecursiva(const char* src, const char* dst) {
  //Certificar que dst existe, senão cria-lo
  struct stat st;
  if (mkdir(dst,  0777) != 0 && errno != EEXIST) {
    fprintf(stderr, "Error creating directory %s: %s\n", dst, strerror(errno));
    return 0;
  }
  //Procurar agora em cada diretorio
  DIR* direntDIR = opendir(src);
  if (!direntDIR) {
    fprintf(stderr, "datacenter_configure: Failed opening input directory.\n");
    return 0;
  }
  struct dirent* dir;
  int ok = 1;
  while (1) {
    errno = 0;
    dir = readdir(direntDIR);
    if (!dir) {
      if (errno != 0) {
        fprintf(stderr, "Error reading directory %s: %s\n", src, strerror(errno));
        ok = 0;
      }
      break;
    }
    if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0)
      continue;
    char srcPath[PATH_MAX];
    char dstPath[PATH_MAX];

    if (snprintf(srcPath, sizeof(srcPath), "%s/%s", src, dir->d_name) >= (int)sizeof(srcPath) ||
        snprintf(dstPath, sizeof(dstPath), "%s/%s", dst, dir->d_name) >= (int)sizeof(dstPath)) {
      fprintf(stderr, "Path too long in %s\n", src);
      ok = 0;
      break;
        }
    if (path_exists(srcPath)) {
      if (!copiaRecursiva(srcPath, dstPath)) {
        ok = 0;
        break;
      }
    } else if (file_exists(srcPath)) {
      //Se for um file é precisso copiar o ficheiro e mete-lo no dst
    }
  }

  closedir(direntDIR);
  if (errno != 0) {
    fprintf(stderr,"Error closing directory: %s",src);
    return 0;
  }
  return ok;
}