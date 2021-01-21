#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <time.h>
#include <inttypes.h>

struct block_t;
struct arena_t;

typedef struct __attribute__((packed)) arena_t {
	size_t num_pages;
	struct arena_t *previous;
	struct arena_t *next;
	struct block_t *first_block;
} arena_t;

typedef struct __attribute__((packed)) block_t {
	size_t size;
	arena_t *arena;
	struct block_t *previous;
	struct block_t *next;
} block_t;

static const size_t PAGE_SIZE = 4096; /* bytes */
static const size_t DWORD     = 8;    /* bytes */
static arena_t *first_arena   = NULL;

/*
 * Rounds upwards.
 */
static inline size_t
round_size(size_t num, size_t alignment)
{
	return ((num - 1) | (alignment - 1)) + 1;
}

/*
 * Returns 0 in case of size overflow.
 */
static inline size_t
get_number_of_pages_for_block(size_t block_size)
{
	size_t total_bytes = 0;

	if (block_size > SIZE_MAX - sizeof(arena_t))
		return 0;
	total_bytes = block_size + sizeof(arena_t);

	return round_size(total_bytes, PAGE_SIZE) / PAGE_SIZE;
}

static inline size_t
calculate_block_size(size_t size)
{
	size_t block_size = 0;

	size = round_size(size, DWORD);
	block_size = size + sizeof(block_t);

	return block_size;
}

static inline void *
get_block_data_ptr(block_t *restrict block)
{
	return (char *) block + sizeof(block_t);
}

static void
remove_arena(arena_t *arena)
{
	if (arena->previous) {
		if (arena->next) {
			arena->previous->next = arena->next;
			arena->next->previous = arena->previous;
		} else {
			arena->previous->next = NULL;
		}
	} else { /* This is the first arena. */
		if (arena->next) {
			first_arena = arena->next;
			arena->next->previous = NULL;
		} else {
			first_arena = NULL;
		}
	}

	munmap(arena, arena->num_pages * PAGE_SIZE);
}

static arena_t *
new_arena(size_t num_pages)
{
	arena_t arena, *arena_p = NULL, *arena_i = NULL;

	arena.num_pages = num_pages;
	arena.previous = NULL;
	arena.next = NULL;
	arena.first_block = NULL;

	arena_p = mmap(NULL,
	                 arena.num_pages * PAGE_SIZE,
	                 PROT_READ | PROT_WRITE,
	                 MAP_PRIVATE | MAP_ANONYMOUS,
	                 -1,
	                 0);
	if (arena_p == MAP_FAILED)
		return NULL;
	memcpy(arena_p, &arena, sizeof(arena));
	if (!first_arena) {
		first_arena = arena_p;
		return arena_p;
	}
	arena_i = first_arena;
	while (arena_i->next)
		arena_i = arena_i->next;
	arena_i->next = arena_p;
	arena_p->previous = arena_i;

	return arena_p;
}

static arena_t *
allocate_arena_for_block(size_t block_size)
{
	size_t num_pages = get_number_of_pages_for_block(block_size);
	arena_t *arena = NULL;

	if (!num_pages)
		return NULL;
	arena = new_arena(num_pages);
	if (!arena)
		return NULL;

	return arena;
}

static void *
look_for_region(arena_t *arena, size_t needed_space)
{
	size_t arena_size = arena->num_pages * PAGE_SIZE;
	block_t *block = NULL;
	void *region = NULL;

	block = arena->first_block;
	while (block && block->next) {
		region = (char *) block + block->size;
		/* There is enough space in between two blocks. */
		if ((char *) region + needed_space <= (char *) block->next)
			return region;
		block = block->next;
	}

	region = (char *) block + block->size;
	/* There is enough space after all blocks. */
	if ((char *) region + needed_space <= (char *) arena + arena_size)
		return region;

	return NULL;
}

static block_t *
put_block_in_arena(block_t *block)
{
	void *region = NULL;
	block_t *block_p = NULL, *block_i = NULL;

	block->arena = first_arena;
	/* Looks for arena with a suitable region for the new block. */
	while (block->arena) {
		region = look_for_region(block->arena, block->size);
		if (region) {
			memcpy(region, block, sizeof(block_t));
			block_p = region;
			block_i = block_p->arena->first_block;
			/* Add the new block to the linked list. The new block
			 * can either be between two existing blocks or after
			 * all blocks in the arena. */
			while (block_i->next && block_i->next < block_p)
				block_i = block_i->next;
			if (block_i->next)
				block_p->next = block_i->next;
			else
				block_p->next = NULL;
			if (block_i->next) {
				/* Only for new blocks in between two existing
				 * blocks */
				block_i->next->previous = block_p;
			}
			block_p->previous = block_i;
			block_i->next = block_p;
			return block_p;
		}

		block->arena = block->arena->next;
	}

	block->arena = allocate_arena_for_block(block->size);
	if (!block->arena)
		return NULL;
	block_p = (void *) ((char *) block->arena + sizeof(arena_t));
	memcpy(block_p, block, sizeof(block_t));
	block->arena->first_block = block_p;

	return block_p;
}

static block_t *
new_block(size_t block_size)
{
	block_t block = {0}, *block_p = NULL;

	block.size = block_size;
	block.previous = NULL;
	block.next = NULL;

	block_p = put_block_in_arena(&block);

	return block_p;
}

void
m_free(void *ptr)
{
	block_t *block = (block_t *) ((char *) ptr - sizeof(block_t));
	arena_t *arena = block->arena;

	if (block->previous) {
		if (block->next) {
			block->previous->next = block->next;
			block->next->previous = block->previous;
		} else {
			block->previous->next = NULL;
		}
	} else {
		if (block->next) {
			arena->first_block = block->next;
			block->next->previous = NULL;
		} else {
			remove_arena(arena);
		}
	}
}

void *
m_malloc(size_t size)
{
	block_t *block = NULL;
	size_t block_size = 0;
	void *ptr = NULL;

	if (!size)
		return NULL;
	block_size = calculate_block_size(size);

	block = new_block(block_size);
	if (!block)
		return NULL;

	ptr = get_block_data_ptr(block);
	return ptr;
}

void *
m_realloc(void *ptr, size_t size)
{
	block_t *block = (block_t *) ((char *) ptr - sizeof(block_t));
	size_t old_size;
	void *new_ptr = NULL;

	if (!ptr) {
		new_ptr = m_malloc(size);
		return new_ptr;
	} else if (!size) {
		m_free(ptr);
		return NULL;
	}

	old_size = block->size - sizeof(block_t);
	m_free(ptr);
	new_ptr = m_malloc(size);
	memcpy(new_ptr, ptr, old_size);

	return new_ptr;
}

void *
m_calloc(size_t nmemb, size_t size)
{
	size_t num_bytes = 0;
	void *ptr = NULL;

	/* Overflow. */
	if (nmemb > SIZE_MAX / size)
		return NULL;
	num_bytes = nmemb * size;
	ptr = m_malloc(num_bytes);
	if (!ptr)
		return NULL;
	memset(ptr, 0, num_bytes);

	return ptr;
}