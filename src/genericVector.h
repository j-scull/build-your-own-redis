#ifndef GENERICVECTOR_H
#define GENERICVECTOR_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// see https://github.com/dipeshkaphle/genericVector/blob/master/src/genericVector.h

struct Vector {
  void *arr;
  unsigned int word_size;
  unsigned int size;
  unsigned int capacity;
  // int (*cmp)(const void *, const void *);
};
typedef struct Vector Vector;

// returns the number of elements in the vector
unsigned int size(Vector *vec);

// returns the capacity of the vector to hold elements
unsigned int capacity(Vector *vec);

// tells you if the Vector is empty
int is_empty(Vector *vec);

// pushes the data to the end of the array. Resizes if needed.
void push_back(Vector *vec, void *data);

// pushes back data of lenght len
void push_back_multiple(Vector *vec, void *data, size_t len);

// pops the element at the end of the array
// The number will be there as is. It wont be removed but will be overwritten
// next time you do push_back
void pop_back(Vector *vec);

// Vector initializer
Vector *make_vector(unsigned int word_size);

// Reserves the needed amount of memory and returns a heap allocated vector
Vector *init_and_reserve(unsigned int word_size, unsigned int capacity);

// deletes the heap allocated memory
void delete_vec(Vector *vec);

void *get(Vector *vec, unsigned int index);

void *set(Vector *vec, void *data, unsigned int index);

// shrinks the Vector to the size of the vector. No unwanted storage used
void shrink_to_fit(Vector *vec);

// Insert an element at arbitrary position
// void insert(Vector *vec, void *elemAddr, unsigned int position);
void insert(Vector *vec, void *pos, void *data, size_t len);

// Delete an element from an arbitrary position
void remove_at(Vector *vec, unsigned int position);

void print_all(Vector *vec, void (*printFunc)(const void *),
              char printNewLineAtTheEnd);

// Returns a pointer to the underlying array serving as element storage.
void *data(Vector *vec);

// Resizes the container to contain count elements: 
void resize(Vector *vec, unsigned int count);

// Erases the specified elements from the container. 
void erase(Vector *vec, void *begin, void *end);

// return a pointer to the beginning of the Vector
void *begin(Vector *vec);

// return a pointer to the end of the Vector
void *end(Vector *vec);

// remove all elements from the vector and reduce the size to 0
void clear(Vector *vec);

void print_vector_debug(Vector *vec);

#endif