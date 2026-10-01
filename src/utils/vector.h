/**
 *
 * Armazena elementos de tamanho fixo (definido em tempo de execução) em um
 * bloco contíguo de memória que cresce e encolhe automaticamente.
 *
 * Exemplo de uso:
 * @code
 * Vector v;
 * if (!initVector(&v, sizeof(int))) return 1;
 *
 * int x = 42;
 * push_back(&v, &x);
 *
 * int y;
 * pop_back(&v, &y);   // y == 42
 *
 * destroyVector(&v);
 * @endcode
 *
 * @note O vetor guarda CÓPIAS dos elementos (memcpy). Se os elementos
 *       contiverem ponteiros para memória própria, o chamador é responsável
 *       por liberá-la antes de remover os elementos ou destruir o vetor.
 * @note Esta estrutura não é thread-safe.
 */

#ifndef VECTOR_H
#define VECTOR_H

/**
 * @def INITIAL_RESERVED_MEMORY
 * @brief Capacidade inicial (em número de elementos) e capacidade mínima.
 *
 * O vetor nunca encolhe abaixo deste valor.
 */
#ifndef INITIAL_RESERVED_MEMORY
#define INITIAL_RESERVED_MEMORY 8
#include <stddef.h>
#endif

/**
 * @def INITIAL_RESERVATION_MULTIPLIER
 * @brief Fator de crescimento/redução da capacidade.
 *
 * Deve ser >= 2. Com valor 1 a capacidade não cresce de forma útil e com
 * valor 0 ocorre divisão por zero em pop_back().
 */
#ifndef INITIAL_RESERVATION_MULTIPLIER
#define INITIAL_RESERVATION_MULTIPLIER 2
#endif

/**
 * @struct Vector
 * @brief Estrutura do vetor dinâmico.
 *
 * Os campos são públicos por simplicidade, mas devem ser tratados como
 * somente leitura pelo código cliente. Use as funções da API para modificá-los.
 */
typedef struct {
    void*  data;                  /**< Bloco de memória com os elementos (NULL se destruído). */
    size_t size;                  /**< Número de elementos atualmente armazenados. */
    size_t capacity;              /**< Número de elementos que cabem no bloco atual. */
    size_t dataSize;              /**< Tamanho, em bytes, de cada elemento. */
    size_t reservationMultiplier; /**< Fator de crescimento/redução da capacidade (>= 2). */
} Vector;

/**
 * @brief Inicializa um vetor vazio.
 *
 * Reserva INITIAL_RESERVED_MEMORY elementos e define o fator de crescimento
 * como INITIAL_RESERVATION_MULTIPLIER.
 *
 * @param vector    Ponteiro para o vetor a inicializar (não pode ser NULL).
 * @param dataSize  Tamanho em bytes de cada elemento (ex.: sizeof(int)). Deve ser > 0.
 * @return 1 em caso de sucesso; 0 se a alocação de memória falhar.
 *
 * @warning Não chame sobre um vetor já inicializado sem antes chamar
 *          destroyVector(), ou haverá vazamento de memória.
 */
int initVector(Vector* vector, size_t dataSize);

/**
 * @brief Libera a memória do vetor e o deixa em estado vazio.
 *
 * Após a chamada, data == NULL, size == 0 e capacity == 0. É seguro chamar
 * esta função mais de uma vez sobre o mesmo vetor.
 *
 * @param vector Ponteiro para o vetor a destruir.
 *
 * @note Não libera memória apontada pelos próprios elementos.
 */
void destroyVector(Vector* vector);

/**
 * @brief Redimensiona o bloco de memória para a capacidade indicada.
 *
 * Usa realloc(), portanto o conteúdo existente é preservado. A capacidade
 * solicitada é elevada a INITIAL_RESERVED_MEMORY se for menor. A memória
 * adicional NÃO é inicializada com zeros.
 *
 * @param v            Ponteiro para o vetor.
 * @param newCapacity  Nova capacidade desejada, em número de elementos.
 * @return 1 em caso de sucesso; 0 se a nova capacidade for menor que
 *         v->size ou se o realloc falhar. Em caso de falha, o vetor
 *         permanece inalterado e válido.
 */
int reallocVector(Vector* v, size_t newCapacity);

/**
 * @brief Obtém um ponteiro direto para o elemento na posição indicada.
 *
 * @param v      Ponteiro para o vetor.
 * @param index  Índice do elemento (deve ser < v->size).
 * @return Ponteiro para o elemento dentro do vetor.
 *
 * @warning Não há verificação de limites. O ponteiro é invalidado por
 *          qualquer operação que possa realocar o vetor (push_back,
 *          pop_back, reallocVector).
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
 * @brief Sobrescreve o elemento na posição indicada com uma cópia de item.
 *
 * Não altera o tamanho do vetor; para adicionar elementos use push_back().
 *
 * @param vector  Ponteiro para o vetor.
 * @param index   Índice do elemento a sobrescrever (deve ser < vector->size).
 * @param item    Ponteiro para o novo valor (v->dataSize bytes são copiados).
 *
 * @warning Não há verificação de limites nem de ponteiros nulos.
 */
void setToVector(Vector* vector, size_t index, void* item);

/**
 * @brief Adiciona um elemento ao final do vetor.
 *
 * Se o vetor estiver cheio, a capacidade é multiplicada por
 * reservationMultiplier (crescendo ao menos 1 elemento).
 *
 * @param v     Ponteiro para o vetor.
 * @param item  Ponteiro para o elemento a copiar (v->dataSize bytes).
 * @return 1 em caso de sucesso; 0 se não foi possível aumentar a capacidade
 *         (o vetor permanece inalterado).
 *
 * @note Complexidade amortizada O(1).
 */
int push_back(Vector* v, void* item);

/**
 * @brief Remove o último elemento do vetor.
 *
 * Se o vetor ficar com ocupação baixa (size < capacity / multiplier²), a
 * capacidade é reduzida por um fator reservationMultiplier, nunca abaixo de
 * INITIAL_RESERVED_MEMORY. Uma falha ao reduzir a memória é ignorada, pois
 * não afeta a correção da operação.
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

#ifdef __cplusplus
}
#endif

#endif /* VECTOR_H */