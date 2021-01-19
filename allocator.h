#ifndef ALLOCATOR_INCLUDE_GUARD
#define ALLOCATOR_INCLUDE_GUARD

#include <stdlib.h>

void *m_malloc(size_t size);
void *m_calloc(size_t nmemb, size_t size);
void *m_realloc(void *ptr, size_t size);
void m_free(void *ptr);

#endif /* ALLOCATOR_INCLUDE_GUARD */