#include <genericVector.h>
#include <util.h>


unsigned int size(Vector *vec) {
  if (vec == NULL) {
    fprintf(stderr, "Vector is NULL\n");
    exit(1);
  }
  return vec->size; 
}

unsigned int capacity(Vector *vec) { 
  return vec->capacity; 
}

int isempty(Vector *vec) { 
  return (size(vec) == 0) ? 1 : 0; 
}

void push_back(Vector *vec, void *data) {
  if (size(vec) == 0) {
    vec->arr = calloc(2, vec->word_size);
    vec->capacity = 2;
  }
  else if (size(vec) >= capacity(vec)) {
    void *tmp = vec->arr;
    vec->arr = calloc(2 * capacity(vec), vec->word_size);
    vec->capacity = 2 * capacity(vec);
    memcpy(vec->arr, tmp, size(vec) * vec->word_size);
    free(tmp);
  }
  char *ptr = (char *)vec->arr;
  ptr += (size(vec) * (vec->word_size));
  memcpy((void *)ptr, (const void *)data, vec->word_size);
  vec->size++;
}

void push_back_multiple(Vector *vec, void *data, size_t len) {

  int old_size = size(vec);
  int new_size = old_size + len;
  int msb = get_msb(new_size);
  int new_capacity = pow(2, msb + 1);

  if (old_size == 0) {
    vec->arr = calloc(new_capacity, vec->word_size);
    memcpy(vec->arr, data, len * vec->word_size);
  } 
  else {

    void *tmp = vec->arr;
    vec->arr = calloc(new_capacity, vec->word_size);
    memcpy(vec->arr, tmp,  old_size * vec->word_size);
    free(tmp);

    char *ptr = (char *)vec->arr;
    ptr += old_size * vec->word_size;
    memcpy((void *)ptr, data, len * vec->word_size);
  }

  vec->capacity = new_capacity;
  vec->size = new_size;
}

void pop_back(Vector *vec) {
  if (size(vec) != 0) {
    vec->size -= 1;
  }
}

Vector *make_vector(unsigned int word_size) {
  Vector *vec = (Vector *)calloc(1, sizeof(Vector));
  vec->word_size = word_size;
  vec->size = 0;
  vec->capacity = 0;

  return vec;
}

Vector *init_and_reserve(unsigned int word_size, unsigned int capacity) {
  Vector *vec = make_vector(word_size);
  vec->arr = calloc(capacity, word_size);
  vec->capacity = capacity;
  return vec;
}

void delete_vec(Vector *vec) {
  if (vec->arr != NULL) {
    free(vec->arr);
    vec->arr = NULL;
  }
}

void *get(Vector *vec, unsigned int index) {
  if (vec == NULL) {
    fprintf(stderr, "Vector is NULL\n");
    exit(1);
  }
  if (index >= size(vec)) {
    fprintf(stderr, "Index out of bounds\n");
    exit(1);
  }
  
  return (void *)((char *)vec->arr + (index * vec->word_size));
}

void *set(Vector *vec, void *data, unsigned int index) {
  if (index < capacity(vec)) {
    void *ptr = (void *)((char *)vec->arr + (index * (vec->word_size)));
    if (data == NULL) {
      ptr = NULL;
    }
    else {
      memcpy(ptr, (const void *)data, vec->word_size);
      if (index >= vec->size)
        vec->size = (index + 1);
    }
    return ptr;
  }
  return NULL;
}

void shrink_to_fit(Vector *vec) {
  if (size(vec) != 0) {
    void *tmp = vec->arr;
    vec->arr = calloc(vec->size, vec->word_size);
    memcpy(vec->arr, tmp, size(vec) * vec->word_size);
    free(tmp);
    vec->capacity = vec->size;
  }
}

void insert(Vector *vec, void *pos, void *data, size_t len) {
  if (vec == NULL) {
    push_back_multiple(vec, data, len);
    return;
  }
  int index = (pos - vec->arr) / vec->word_size;
  if (index < 0 || index > size(vec)) {
    fprintf(stderr, "Index out of bounds\n");
    exit(1);
  }
  if (index == size(vec)) {
    push_back_multiple(vec, data, len);
  }
  else {

    int old_size = size(vec);
    int new_size = old_size + len;
    int msb = get_msb(new_size);
    int new_capacity = pow(2, msb + 1);

    // create new array
    void *tmp = vec->arr;
    vec->arr = calloc(new_capacity, vec->word_size);

    // copy any existing contents before index
    memcpy(vec->arr, tmp,  index * vec->word_size);

    // copy data
    char *ptr = (char *)vec->arr;
    ptr += index * vec->word_size;
    memcpy((void *)ptr, data, len * vec->word_size);

    // copy any existing contents after index
    int remaining = old_size - index;
    if (remaining > 0) {
      ptr += len * vec->word_size;
      char *tmp_ptr = (char *)tmp;
      tmp_ptr += index * vec->word_size;
      memcpy((void *)ptr, tmp_ptr,  remaining * vec->word_size);
    }

    free(tmp);
    vec->capacity = new_capacity;
    vec->size = new_size;
  }
}

void remove_at(Vector *vec, unsigned int position) {
  int len = size(vec);
  if (position == len - 1) {
    pop_back(vec);
  } else {
    for (unsigned int i = position + 1; i < size(vec); i++) {
      set(vec, get(vec, i), i - 1);
    }
    (vec->size)--;
  }
}

void print_all(Vector *vec, void (*printFunc)(const void *), char printNewLineAtTheEnd) {
  for (unsigned int i = 0; i < size(vec); i++) {
    printFunc(get(vec, i));
  }
  if (printNewLineAtTheEnd) {
    printf("\n");
  }
}

void *data(Vector *vec) {
  return vec->arr;
}

void resize(Vector *vec, unsigned int count) {
  if (vec->arr == NULL) {
    vec->arr = calloc(count, vec->word_size);
    vec->capacity = count;
  } 
  else if (count > capacity(vec)) {
    void *tmp = vec->arr;
    vec->arr = calloc(count, vec->word_size);
    vec->capacity = count;
    memcpy(vec->arr, tmp, size(vec) * vec->word_size);
    free(tmp);
  }
  vec->size = count;
}

void erase(Vector *vec, void *begin, void *end) {
  if (begin < vec->arr || begin > vec->arr + vec->size * vec->word_size) {
      fprintf(stderr, "Begin index out of bounds\n");
      exit(1);
  }
  if (end <= vec->arr || end > vec->arr + (vec->size) * vec->word_size) {
    fprintf(stderr, "End index out of bounds\n");
    exit(1);
  }
  if (begin > end) {
    fprintf(stderr, "Begin not before end\n");
    exit(1);
  }

  int start_index = (begin - vec->arr) / vec->word_size;
  int end_index = (end - vec->arr) / vec->word_size;
  // allocate new array
  void *tmp = vec->arr;
  vec->arr = calloc(vec->capacity, vec->word_size);

  // copy over elements before begin
  memcpy(vec->arr, tmp, start_index * vec->word_size);

  // copy over element after end
  memcpy(vec->arr + start_index * vec->word_size, end, (size(vec) - end_index) * vec->word_size);

  vec->size = vec->size - (end_index - start_index);
  free(tmp);
}

void *begin(Vector *vec) {
  return (void *)((char *)vec->arr);
}

void *end(Vector *vec) {
  if (size(vec) == 0) {
    return (void *)((char *)vec->arr);
  }
  return (void *)((char *)(vec->arr + size(vec) * vec->word_size));
};

void clear(Vector *vec) {
  int z = 0;
  for (unsigned int i = 0; i < size(vec); i++) {
    set(vec, (void *)&z, i);
  }
  vec->size = 0;
}

