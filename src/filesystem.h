#ifndef FILESYSTEM__H
#define FILESYSTEM__H

#include <stddef.h>

/**
 * Um ficheiro sera copiado em chunks de esta quantidade de bytes.
 * Isto por que copiar byte por byte de um ficheiro a outro é demasiado lento mas tambem levar todos byte sde um ficheiro á ram ao mesmo tmepo pode ser uma má ideia se o ficheiro for demasiado pesado.
 * Caso a função falhar o ressultado quanto a existencia do ficheiro de destino e se foi copiado cada byte de forma exata é impredecivel
 */
#define FILE_COPY_CHUNK_SIZE 8192

/**
 * Checks whether a path exists and is a directory.
 *
 * @param path Directory path.
 *
 * @return 1 if it exists and is a directory, 0 otherwise.
 */
int path_exists(const char *path);

/**
 * Checks whether a path exists and is a regular file.
 *
 * @param path File path.
 *
 * @return 1 if it exists and is a regular file, 0 otherwise.
 */
int file_exists(const char *path);

/**
 * Converts the given path into an absolute path, resolving symbolic links,
 * relative components ('.' and '..'), and redundant separators. The resolved
 * path is copied into the provided buffer.
 *
 * @param path Path to resolve.
 * @param buffer Destination buffer where the absolute path will be stored.
 * @param size Size of the destination buffer, in bytes.
 *
 * @return 0 if the path was successfully resolved and copied to the buffer
 * @return 1 if the path could not be resolved or the buffer is too small.
 */
int absolute_path(const char *path, char *buffer, size_t size);

//@r

/**
 * @details
 * Tem de se garantir que a diretoria src já exista.
 * Cria a diretoria dst caso não exista.
 * Caso a copia recursiva tenha falhado em alguma etapa o diretorio de destino pode ficar medio construido pelo que o resultado sera impredecivel
 * @param src Path aonde se vao copiar os ficheiros, tem de se garantir a sua existencia antes da chamada
 * @param dst Path de destino, caso nao exista sera criado
 * @return
 *  1 se a copia recursiva foi bem sucedida para todos os ficheiros
 *  0 se não foi bem sucedida
 */
int copiaRecursiva(const char* src, const char* dst);

/**
 * @details O usuario deve garantir a existencia de srcFile e que a diretoria que ira conter dstFile exista
 * @param srcFile path do ficheiro a ser copiado
 * @param dstFile diretoria em que devera ser colocado a copia do ficheiro (já deve contem o propio nome do ficheiro)
 * @return
 *  1 caso a copia tenha sido bem sucedida
 *  0 caso contrario
 */
int copiaFicheiro(const char* srcFile, const char* dstFile);
#endif // FILESYSTEM__H