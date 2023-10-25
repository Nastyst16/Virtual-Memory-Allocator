// Copyright© 2023 Nastase Cristian-Gabriel 315CAa

#pragma once
#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define MAX_STRING_SIZE 64

#define DIE(assertion, call_description)	\
	do {	\
		if (assertion) {	\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(errno);	\
		}	\
	} while (0)

// double linked list for block list and also
// for miniblock lists;
typedef struct {
	void *head;
	unsigned int data_size;
} list_t;

// arena which has a dll_list of blocks;
typedef struct {
	unsigned long size;
	unsigned long capacity;
	list_t *block_list;
	unsigned long blocks, miniblocks;
} arena_t;

// block which has a dll_list of miniblocks
typedef struct block_t block_t;
struct block_t {
	unsigned long start_address;
	size_t size;
	block_t *prev, *next;
	list_t *miniblock_list;
};

typedef struct miniblock_t miniblock_t;
struct miniblock_t {
	unsigned long start_address;
	size_t size;
	miniblock_t *prev, *next;
	unsigned char perm;
	void *rw_buffer;
	unsigned long buffer_size;
};

list_t *dll_create(void);

// function for ALLOC_BLOCK command;
arena_t *alloc_arena(const unsigned long size);

// function for DEALLOC_ARENA command;
void dealloc_arena(arena_t *arena);

// functions that helps to alloc blocks
// every function represents a special case of allocation;
void alloc_empty_block_list(arena_t *arena, const unsigned long address,
							miniblock_t *miniblock, const unsigned long size);
void alloc_begin_bl_list(arena_t *arena, const unsigned long address,
						 miniblock_t *miniblock, const unsigned long size);
void alloc_merge_bl(arena_t *arena, const unsigned long address,
					const unsigned long size,
					miniblock_t *miniblock, miniblock_t *node, block_t *tmp);
void alloc_between_bl(arena_t *arena, const unsigned long address,
					  unsigned long size, miniblock_t *miniblock, block_t *tmp);
void alloc_end_bl(arena_t *arena, const unsigned long address,
				  const unsigned long size,
				  miniblock_t *miniblock, miniblock_t *node, block_t *tmp);
void alloc_begin_bl(arena_t *arena, const unsigned long address,
					const unsigned long size,
					miniblock_t *miniblock, miniblock_t *node, block_t *tmp);
void alloc_end_bl_list(arena_t *arena, const unsigned long address,
					   unsigned long size, miniblock_t *miniblock,
					   block_t *tmp);
int alloc_block_errors(arena_t *arena, const unsigned long address,
					   const unsigned long size);
miniblock_t *create_miniblock(arena_t *arena, const unsigned long address,
							  const unsigned long size);
void alloc_block(arena_t *arena, const unsigned long address,
				 const unsigned long size);

// functions for FREE_BLOCK command;
// every single one of them represents a special case for free;
void free_bl_with_1miniblock(arena_t *arena, const unsigned long address,
							 block_t *tmp_b, miniblock_t *tmp_mb);
void free_middle_bl(arena_t *arena, const unsigned long address,
					block_t *tmp_b, miniblock_t *tmp_mb);
void free_begin_bl(arena_t *arena, const unsigned long address,
				   block_t *tmp_b, miniblock_t *tmp_mb);
void free_end_bl(arena_t *arena, const unsigned long address,
				 block_t *tmp_b, miniblock_t *tmp_mb);
void free_block(arena_t *arena, const unsigned long address);

// function for READ command;
void read(arena_t *arena, unsigned long address, unsigned long size);

// function for WRITE command;
void write(arena_t *arena, const unsigned long address,
		   const unsigned long size, int8_t *data);

// function for PMAP command
void pmap(const arena_t *arena);

// function for MPROTECT command/ bonus;
void mprotect(arena_t *arena, unsigned long address, uint8_t permission);
