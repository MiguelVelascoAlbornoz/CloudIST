#include "vector.h"

#include <stdlib.h>
#include <string.h>

int initVector(Vector* vector, size_t dataSize) {
    vector->capacity = INITIAL_RESERVED_MEMORY;
    vector->dataSize = dataSize;
    vector->size = 0;
    vector->reservationMultiplier = INITIAL_RESERVATION_MULTIPLIER;
    vector->data = (void*) calloc(vector->capacity,dataSize);
    if (!vector->data) return 0;



    return 1;
}
void destroyVector(Vector* vector) {
    if (vector->data) free(vector->data);
    vector->data = NULL;

    vector->size = 0;
    vector->capacity = 0;
}

int reallocVector(Vector* v, size_t newCapacity) {
    if (v->reservationMultiplier >2) return 0;
    if (newCapacity < INITIAL_RESERVED_MEMORY) newCapacity = INITIAL_RESERVED_MEMORY;
    if (newCapacity < v->size) return 0;

    void* newMem = realloc(v->data, v->dataSize*newCapacity);
    if (!newMem) return 0;
    v->data = newMem;
    v->capacity = newCapacity;
    return 1;
}
void* getAtVector(Vector* v, size_t index) {
    return (char*)v->data + index * v->dataSize;
}
void getFromVector(Vector* v, size_t index, void* out) {
    memcpy(out, (char*)v->data + index * v->dataSize, v->dataSize);
}

void setToVector(Vector* vector, size_t index, void* item) {
    memcpy(getAtVector(vector, index), item, vector->dataSize);
}
int push_back(Vector* v, void* item) {
    if (v->size >= v->capacity) {
        if (!reallocVector(v,v->capacity * v->reservationMultiplier)) {
            return 0;
        }
    }
    setToVector(v, v->size, item);
    v->size++;
    return 1;
}



int pop_back(Vector* v, void* out) {
    if (v->size == 0) return 0;
    v->size--;
    if (out) getFromVector(v, v->size, out);   // copia ANTES de reducir

    if (v->size < v->capacity / (v->reservationMultiplier * v->reservationMultiplier)) {

        if (!reallocVector(v,v->capacity / v->reservationMultiplier)) {
            v->size++;
            return 0;
        }
    }
    return 1;
}
