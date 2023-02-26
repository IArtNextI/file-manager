#pragma once

#include <inttypes.h>
#include <stdlib.h>

#define VECTOR(type)                                                           \
    typedef struct vector_##type {                                             \
        type *data;                                                            \
        int64_t size;                                                          \
        int64_t capacity;                                                      \
    } vector_##type;                                                           \
                                                                               \
    static vector_##type *create_##type(size_t n, type el) {                   \
        vector_##type *res = (vector_##type *)calloc(1, sizeof(*res));         \
        res->data = (type *)calloc(n, sizeof(type));                           \
        for (int i = 0; i < n; ++i) {                                          \
            res->data[i] = el;                                                 \
        }                                                                      \
        res->size = n;                                                         \
        res->capacity = n;                                                     \
        return res;                                                            \
    }                                                                          \
                                                                               \
    static vector_##type *reserve_##type(size_t n, type el) {                  \
        vector_##type *res = (vector_##type *)calloc(1, sizeof(*res));         \
        if (!res) {                                                            \
            return NULL;                                                       \
        }                                                                      \
        res->data = (type *)calloc(n, sizeof(type));                           \
        if (!res->data) {                                                      \
            free(res);                                                         \
            return NULL;                                                       \
        }                                                                      \
        for (int i = 0; i < n; ++i) {                                          \
            res->data[i] = el;                                                 \
        }                                                                      \
        res->size = 0;                                                         \
        res->capacity = n;                                                     \
        return res;                                                            \
    }                                                                          \
                                                                               \
    static vector_##type *reserve_junk_##type(size_t n) {                      \
        vector_##type *res = (vector_##type *)calloc(1, sizeof(*res));         \
        if (!res) {                                                            \
            return NULL;                                                       \
        }                                                                      \
        res->data = (type *)calloc(n, sizeof(type));                           \
        if (!res->data) {                                                      \
            free(res);                                                         \
            return NULL;                                                       \
        }                                                                      \
        res->size = 0;                                                         \
        res->capacity = n;                                                     \
        return res;                                                            \
    }                                                                          \
                                                                               \
    static type get_##type(vector_##type *vector, size_t ind) {                \
        return vector->data[ind];                                              \
    }                                                                          \
    static void set_##type(vector_##type *vector, size_t ind, type el) {       \
        vector->data[ind] = el;                                                \
    }                                                                          \
    static bool push_back_##type(vector_##type *vector, type el) {             \
        if (vector->size < vector->capacity) {                                 \
            vector->data[vector->size++] = el;                                 \
        } else {                                                               \
            type *new_ptr = (type *)reallocarray(                              \
                (void *)vector->data, vector->capacity * 2, sizeof(type));     \
            if (new_ptr == NULL) {                                             \
                return false;                                                  \
            } else {                                                           \
                vector->data = new_ptr;                                        \
                vector->capacity *= 2;                                         \
            }                                                                  \
            vector->data[vector->size++] = el;                                 \
        }                                                                      \
        return true;                                                           \
    }                                                                          \
    static bool create_back_##type(vector_##type *vector) {                    \
        if (vector->size < vector->capacity) {                                 \
            vector->size++;                                                    \
        } else {                                                               \
            type *new_ptr = (type *)reallocarray(                              \
                (void *)vector->data, vector->capacity * 2, sizeof(type));     \
            if (new_ptr == NULL) {                                             \
                return false;                                                  \
            } else {                                                           \
                vector->data = new_ptr;                                        \
                vector->capacity *= 2;                                         \
            }                                                                  \
            vector->size++;                                                    \
        }                                                                      \
        return true;                                                           \
    }                                                                          \
    static void pop_back_##type(vector_##type *vector) { vector->size--; }     \
    static type* back_##type(vector_##type *vector) {                          \
        return &vector->data[vector->size - 1];                                \
    }                                                                          \
    static void free_##type(vector_##type *vector) {                           \
        free(vector->data);                                                    \
        free(vector);                                                          \
    }                                                                          \
    static bool double_in_size_##type(vector_##type *vector) {                 \
        type *new_ptr = (type *)reallocarray(                                  \
            (void *)vector->data, vector->capacity * 2, sizeof(type));         \
        if (new_ptr == NULL) {                                                 \
            return false;                                                      \
        } else {                                                               \
            vector->data = new_ptr;                                            \
            vector->capacity *= 2;                                             \
        }                                                                      \
        return true;                                                           \
    }
