#define CSTVECTOR_H_INCLUDED

// copied some of std::vector's functions for this data type (only copied the args but not the code)

typedef struct cstvector
{
    void *data;
    size_t size;
    size_t capacity;
    size_t elem_size;
} cstvector_t;

int cstvector_init(cstvector_t *vec, size_t elem_size);
int cstvector_reserve(cstvector_t *vec, size_t newsize);
int cstvector_resize(cstvector_t *vec, size_t newsize, void *newfill);
void cstvector_destroy(cstvector_t *vec);

void *cstvector_back(cstvector_t *vec);
void *cstvector_front(cstvector_t *vec);

int cstvector_push_back(cstvector_t *vec, void *value);
int cstvector_pop_back(cstvector_t *vec);

void *cstvector_getat(cstvector_t *vec, size_t index);
int cstvector_setat(cstvector_t *vec, size_t index, void *value);

void cstvector_clear(cstvector_t *vec);
int cstvector_insert(cstvector_t *vec, size_t index, void *value);
int cstvector_erase(cstvector_t *vec, size_t index);
