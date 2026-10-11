
#ifndef VECTOR_H
#define VECTOR_H

/**
 * @def INITIAL_RESERVED_MEMORY
 * @brief Capacidade inicial (em número de elementos) e capacidade mínima.
 *
 * O vetor nunca encolhe abaixo deste valor.
 */
#define INITIAL_RESERVED_MEMORY 8
#include <stddef.h>

/**
 * @def INITIAL_RESERVATION_MULTIPLIER
 * @brief Fator de crescimento/redução da capacidade.
 *
 * Deve ser >= 2. Com valor 1 a capacidade não cresce de forma útil e com
 */

#define INITIAL_RESERVATION_MULTIPLIER 2


/**
 * @struct Vector
 * @brief Estrutura do vetor dinâmico.
 * Não é bom mudar nenhuma das variaveis deste struct que não seja reservationMultiplier
 */
typedef struct {
    void*  data;                  /**< Bloco de memória com os elementos (NULL se destruído). */
    size_t size;                  /**< Número de elementos atualmente armazenados. */
    size_t capacity;              /**< Número de elementos que cabem no bloco atual. */
    size_t dataSize;              /**< Tamanho, em bytes, de cada elemento. */
    size_t reservationMultiplier; /**< Fator de crescimento/redução da capacidade (>= 2). */
} Vector;

/**
 * Reserva INITIAL_RESERVED_MEMORY elementos e define o fator de crescimento
 *
 * @param vector    Ponteiro para o vetor a inicializar (não pode ser NULL).
 * @param dataSize  Tamanho em bytes de cada elemento (ex.: sizeof(int)). Deve ser > 0.
 * @return 1 em caso de sucesso; 0 se a alocação de memória falhar.

 */
int initVector(Vector* vector, size_t dataSize);

/**
 * @brief Libera a memória do vetor
 * @param vector Ponteiro para o vetor a destruir.
 */
void destroyVector(Vector* vector);

/**
 * @brief Redimensiona o bloco de memória para a capacidade indicada.
 *
 * @param v            Ponteiro para o vetor.
 * @param newCapacity  Nova capacidade desejada, em número de elementos.
 * @return 1 em caso de sucesso; 0 se realloc falhar
 */
int reallocVector(Vector* v, size_t newCapacity);

/**
 * @brief Obtém um ponteiro direto para o elemento na posição indicada.
 *
 * @param v      Ponteiro para o vetor.
 * @param index  Índice do elemento (deve ser < v->size).
 * @return Ponteiro para o elemento dentro do vetor.
 */
void* getAtVector(Vector* v, size_t index);

/**
 * @brief Copia o elemento na posição indicada para um buffer externo.
 *
 * @param v      Ponteiro para o vetor.
 * @param index  Índice do elemento (deve ser < v->size).
 * @param out    Destino da cópia; deve ter pelo menos v->dataSize bytes.
 *
 * @warning Não há verificação de limites nem de ponteiros nulos.
 */
void getFromVector(Vector* v, size_t index, void* out);

/**
 * @brief Escreve um valor no vetor
 *
 * @param vector  Ponteiro para o vetor.
 * @param index   Índice do elemento a sobrescrever (deve ser < vector->size).
 * @param item    Ponteiro para o novo valor (v->dataSize bytes são copiados).
 */
void setToVector(Vector* vector, size_t index, void* item);

/**
 * @brief Adiciona um elemento ao final do vetor. Esta função copia item pelo que se o item original for destruido a copia neste vetor continua a exsitir
 *
 * Se o vetor estiver cheio, a capacidade é multiplicada por
 * reservationMultiplier (crescendo ao menos 1 elemento).
 *
 * @param v     Ponteiro para o vetor.
 * @param item  Ponteiro para o elemento a copiar (v->dataSize bytes).
 * @return 1 em caso de sucesso; 0 se não foi possível aumentar a capacidade
 *         (o vetor permanece inalterado).
 */
int push_back(Vector* v, void* item);

/**
 * @brief Remove o último elemento do vetor.
 *
 * @param v    Ponteiro para o vetor.
 * @param out  Destino opcional para uma cópia do elemento removido (deve ter
 *             pelo menos v->dataSize bytes). Pode ser NULL para descartá-lo.
 * @return 1 se um elemento foi removido; 0 se o vetor estava vazio.
 *
 * @note Se os elementos possuírem memória própria, libere-a usando o valor
 *       copiado em out.
 */
int pop_back(Vector* v, void* out);

#endif
