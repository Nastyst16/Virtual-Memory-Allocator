// Copyright© 2023 Nastase Cristian-Gabriel 315CAa

#include "vma.h"

list_t*
dll_create()
{
	list_t *list = malloc(sizeof(list_t));
	DIE(!list, "Malloc failed\n");

	list->head = NULL;

	return list;
}

arena_t *alloc_arena(const unsigned long size)
{
	arena_t *arena = malloc(sizeof(*arena));
	DIE(!arena, "Malloc failed\n");

	arena->size = 0;
	arena->capacity = size;
	arena->block_list = NULL;
	arena->blocks = 0;
	arena->miniblocks = 0;

	return arena;
}

void dealloc_arena(arena_t *arena)
{
	// this while frees the first miniblock
	// from the arena until it is empty;
	while (arena->miniblocks != 0) {
		block_t *block = arena->block_list->head;
		free_block(arena, *(unsigned long *)block->miniblock_list->head);
	}
	free(arena);
}

// special case for ALLOC_BLOCK command
// it allocs a miniblock and a block if the arena is empty;
void alloc_empty_block_list(arena_t *arena, const unsigned long address,
							miniblock_t *miniblock, const unsigned long size)
{
	arena->block_list = dll_create();
	block_t *block = malloc(sizeof(*block));
	DIE(!block, "Malloc failed\n");

	arena->block_list->head = block;
	block->prev = block;
	block->next = block;
	block->start_address = address;
	block->size = size;
	block->miniblock_list = dll_create();
	block->miniblock_list->head = miniblock;
	miniblock->prev = miniblock;
	miniblock->next = miniblock;

	++arena->blocks;
	++arena->miniblocks;

	arena->size += size;
}

// special case for ALLOC_BLOCK command
// this allocs a block & miniblock at the beginning of the block list;
void alloc_begin_bl_list(arena_t *arena, const unsigned long address,
						 miniblock_t *miniblock, const unsigned long size)
{
	block_t *block = malloc(sizeof(*block));
	DIE(!block, "Malloc failed\n");

	block_t *tmp = arena->block_list->head;
	block->next = tmp;
	block->prev = tmp->prev;
	tmp->prev->next = block;
	tmp->prev = block;
	arena->block_list->head = block;
	block->miniblock_list = dll_create();
	block->miniblock_list->head = miniblock;
	miniblock->prev = miniblock;
	miniblock->next = miniblock;

	block->start_address = address;
	block->size = size;

	++arena->blocks;
	++arena->miniblocks;
}

// special case for ALLOC_BLOCK command
// this allocs a block & miniblock at the end of the block list;
void alloc_end_bl_list(arena_t *arena, const unsigned long address,
					   unsigned long size, miniblock_t *miniblock, block_t *tmp)
{
	block_t *block = malloc(sizeof(*block));
	DIE(!block, "Malloc failed\n");

	block->miniblock_list = dll_create();
	block->next = tmp->next;
	block->next->prev = block;
	tmp->next = block;
	block->prev = tmp;
	block->miniblock_list->head = miniblock;
	miniblock->next = miniblock;
	miniblock->prev = miniblock;
	block->start_address = address;
	block->size = size;

	++arena->blocks;
	++arena->miniblocks;
}

// special case for ALLOC_BLOCK command
// this function merge 2 blocks which are united by a miniblock;
void alloc_merge_bl(arena_t *arena, const unsigned long address,
					const unsigned long size,
					miniblock_t *miniblock, miniblock_t *node, block_t *tmp)
{
	tmp->size =
	tmp->next->start_address + tmp->next->size - tmp->start_address;

	miniblock->next = tmp->next->miniblock_list->head;
	miniblock->next->prev->next = node;
	node->prev->next = miniblock;
	miniblock->prev = node->prev;
	node->prev = miniblock->next->prev;
	miniblock->next->prev = miniblock;

	block_t *block_free = tmp->next;
	tmp->next = block_free->next;
	block_free->next->prev = tmp;

	--arena->blocks;
	++arena->miniblocks;

	free(block_free->miniblock_list);
	free(block_free);
}

// special case for ALLOC_BLOCK command
// this function alloc block & miniblock between 2 blocks;
void alloc_between_bl(arena_t *arena, const unsigned long address,
					  unsigned long size, miniblock_t *miniblock, block_t *tmp)
{
	block_t *block = malloc(sizeof(*block));
	DIE(!block, "Malloc failed\n");

	block->next = tmp->next;
	tmp->next = block;
	block->prev = tmp;
	block->next->prev = block;
	block->miniblock_list = dll_create();
	block->miniblock_list->head = miniblock;
	miniblock->next = miniblock;
	miniblock->prev = miniblock;
	block->start_address = address;
	block->size = size;

	++arena->miniblocks;
	++arena->blocks;
}

// special case for ALLOC_BLOCK command
// this function allocs a miniblock
// at the end of a miniblock list;
void alloc_end_bl(arena_t *arena, const unsigned long address,
				  const unsigned long size,
				  miniblock_t *miniblock, miniblock_t *node, block_t *tmp)
{
	node->prev->next = miniblock;
	miniblock->prev = node->prev;
	miniblock->next = node;
	node->prev = miniblock;

	tmp->size += size;
	++arena->miniblocks;
}

// special case for ALLOC_BLOCK command
// this function allocs a miniblock
// at the beginning of a miniblock list;
void alloc_begin_bl(arena_t *arena, const unsigned long address,
					const unsigned long size,
					miniblock_t *miniblock, miniblock_t *node, block_t *tmp)
{
	tmp->miniblock_list->head = miniblock;
	miniblock->next = node;
	miniblock->prev = node->prev;
	node->prev->next = miniblock;

	tmp->size = tmp->start_address + tmp->size - address;
	tmp->start_address = address;
	++arena->miniblocks;
}

// errors for ALLOC_BLOCK command
int alloc_block_errors(arena_t *arena, const unsigned long address,
					   const unsigned long size)
{
	if (!arena) {
		printf("Arena doesnt exists\n");
		return 1;
	}

	if (address >= arena->capacity) {
		printf("The allocated address is outside the size of arena\n");
		return 1;
	}

	if (address + size > arena->capacity) {
		printf("The end address is past the size of the arena\n");
		return 1;
	}

	return 0;
}

// this function creates a miniblock;
miniblock_t *create_miniblock(arena_t *arena, const unsigned long address,
							  const unsigned long size)
{
	miniblock_t *miniblock = malloc(sizeof(*miniblock));
	DIE(!miniblock, "Malloc failed\n");

	miniblock->start_address = address;
	miniblock->size = size;
	miniblock->perm = 6;
	miniblock->rw_buffer = NULL;
}

// this is the main function for ALLOC_BLOCK command
// here all the "alloc" functions from above are used;
void alloc_block(arena_t *arena, const unsigned long address,
				 const unsigned long size)
{
	// verifying the correctness of the input;
	if (alloc_block_errors(arena, address, size))
		return;

	miniblock_t *miniblock = create_miniblock(arena, address, size);

	if (arena->size == 0) {
		// alloc the block list;
		alloc_empty_block_list(arena, address, miniblock, size);
		return;
	}
	arena->size += size;
	block_t *tmp = arena->block_list->head;

	// if for the case when a block is addes
	// at the beginning of the block list
	if (tmp->start_address > miniblock->start_address + miniblock->size + 1) {
		alloc_begin_bl_list(arena, address, miniblock, size);
		return;
	}
	// in this while we are moving between the blocks so we can
	// make changes inside de blocks which are already allocated;
	while (1) {
		miniblock_t *node;
		node = tmp->miniblock_list->head;

		// if for case when a miniblock merges 2 blocks;
		if (tmp->start_address + tmp->size == miniblock->start_address &&
			miniblock->start_address + miniblock->size
			== tmp->next->start_address) {
			alloc_merge_bl(arena, address, size, miniblock, node, tmp);
			return;
		}

		// if the new block is located between 2 blocks
		if (address > tmp->start_address + tmp->size &&
			address + size < tmp->next->start_address) {
			alloc_between_bl(arena, address, size, miniblock, tmp);
			return;
		}

		// if the new miniblock sticks to the end of a block
		if (tmp->start_address + tmp->size == address) {
			if (tmp->next->start_address <
				miniblock->start_address + miniblock->size &&
				tmp->next != arena->block_list->head)
				break;
			alloc_end_bl(arena, address, size, miniblock, node, tmp);
			return;
		}

		// if the new miniblock sticks to the beginning of a block
		if (address + size == tmp->start_address) {
			alloc_begin_bl(arena, address, size, miniblock, node, tmp);
			return;
		}

		// this "if" stops the while if we reached the end of the blocks;
		if (tmp->next == arena->block_list->head)
			break;

		// moving through the list
		tmp = tmp->next;
	}
	// if we passed the while without returning, this meand the node "tmp"
	// is located at the end of the block list;
	if (address > tmp->start_address + tmp->size) {
		alloc_end_bl_list(arena, address, size, miniblock, tmp);
		return;
	}
	arena->size -= size;

	free(miniblock);
	// if none of our "ifs" were called, this means the
	// zone was already allocated;
	printf("This zone was already allocated.\n");
}

// special case for FREE_BLOCK command
// if is freed a block with only a miniblock;
void free_bl_with_1miniblock(arena_t *arena, const unsigned long address,
							 block_t *tmp_b, miniblock_t *tmp_mb)
{
	arena->size -= tmp_mb->size;
	if (tmp_mb->rw_buffer)
		free(tmp_mb->rw_buffer);
	free(tmp_mb);
	--arena->miniblocks;
	--arena->blocks;

	// when only a block exists in the block list;
	if (tmp_b->start_address == tmp_b->next->start_address) {
		free(tmp_b->miniblock_list);
		free(tmp_b);
		free(arena->block_list);

		arena->size = 0;
		return;
	}

	// if the block is located at the
	// beginning of the block list;
	if (tmp_b->start_address <
		tmp_b->prev->start_address + tmp_b->prev->size &&
		tmp_b->start_address + tmp_b->size <
		tmp_b->next->start_address) {
		arena->block_list->head = tmp_b->next;
		tmp_b->next->prev = tmp_b->prev;
		tmp_b->prev->next = tmp_b->next;

		free(tmp_b->miniblock_list);
		free(tmp_b);
		return;
	}

	// if for case when the block is located in
	// the middle or at the end of the block list;
	if (tmp_b->start_address >
		tmp_b->prev->start_address + tmp_b->prev->size) {
		tmp_b->next->prev = tmp_b->prev;
		tmp_b->prev->next = tmp_b->next;
		free(tmp_b->miniblock_list);
		free(tmp_b);
		return;
	}
}

// special case for FREE_BLOCK command
// for the case when a block is splitted in 2
void free_middle_bl(arena_t *arena, const unsigned long address,
					block_t *tmp_b, miniblock_t *tmp_mb)
{
	--arena->miniblocks;
	++arena->blocks;
	arena->size -= tmp_mb->size;

	miniblock_t *aux = tmp_b->miniblock_list->head;
	block_t *block = malloc(sizeof(*block));
	DIE(!block, "Malloc failed\n");

	block->miniblock_list = dll_create();
	block->miniblock_list->head = tmp_mb->next;
	tmp_mb->next->prev = aux->prev;
	aux->prev->next = tmp_mb->next;
	tmp_mb->prev->next = aux;
	aux->prev = tmp_mb->prev;

	block->next = tmp_b->next;
	block->next->prev = block;
	block->prev = tmp_b;
	tmp_b->next = block;

	block->start_address = tmp_mb->next->start_address;

	block->size = tmp_mb->next->prev->start_address +
	tmp_mb->next->prev->size - block->start_address;

	tmp_b->size =
	aux->prev->start_address + aux->prev->size -
	aux->start_address;

	if (tmp_mb->rw_buffer)
		free(tmp_mb->rw_buffer);
	free(tmp_mb);
}

// special case for FREE_BLOCK command
// when a miniblock located at the
// beginning of the miniblock list is freed;
void free_begin_bl(arena_t *arena, const unsigned long address,
				   block_t *tmp_b, miniblock_t *tmp_mb)
{
	tmp_b->start_address = tmp_mb->start_address + tmp_mb->size;
	tmp_b->size -= tmp_mb->size;

	tmp_b->miniblock_list->head = tmp_mb->next;
	tmp_mb->next->prev = tmp_mb->prev;
	tmp_mb->prev->next = tmp_b->miniblock_list->head;

	--arena->miniblocks;
	arena->size -= tmp_mb->size;

	if (tmp_mb->rw_buffer)
		free(tmp_mb->rw_buffer);
	free(tmp_mb);
}

// special case for FREE_BLOCK command
// when a miniblock located at the
// end of a miniblock list is freed;
void free_end_bl(arena_t *arena, const unsigned long address,
				 block_t *tmp_b, miniblock_t *tmp_mb)
{
	miniblock_t *aux = tmp_mb->prev;
	tmp_mb->prev = aux->prev;
	aux->prev->next = tmp_mb;

	tmp_b->size = address - tmp_b->start_address;
	--arena->miniblocks;

	arena->size -= aux->size;
	if (aux->rw_buffer)
		free(aux->rw_buffer);
	free(aux);
}

// this is the main function for FREE_BLOCK command;
// here, all of the above functions are called
// in order to make this command work;
void free_block(arena_t *arena, const unsigned long address)
{
	if (arena->miniblocks == 0) {
		printf("Invalid address for free.\n");
		return;
	}

	block_t *tmp_b = arena->block_list->head;
	// this while helps us travel through the block list;
	while (1) {
		if (tmp_b->start_address <= address) {
			miniblock_t *tmp_mb = tmp_b->miniblock_list->head;

			// if we are freeing a miniblock which is
			// located at the end of the list;
			if (tmp_mb->prev->start_address == address &&
				tmp_mb->prev != tmp_b->miniblock_list->head) {
				free_end_bl(arena, address, tmp_b, tmp_mb);
				return;
			}

			// if we are freeing a miniblock which is
			// located at the beginning of the list;
			if (tmp_b->start_address == address &&
				tmp_mb->start_address + tmp_mb->size ==
				tmp_mb->next->start_address) {
				free_begin_bl(arena, address, tmp_b, tmp_mb);
				return;
			}

			int i = 1;
			while (1) {
				// if the block has only a miniblock;
				if (tmp_mb->start_address == tmp_b->start_address &&
					tmp_mb->start_address == address &&
					tmp_mb->start_address + tmp_mb->size ==
					tmp_b->start_address + tmp_b->size) {
					free_bl_with_1miniblock(arena, address, tmp_b, tmp_mb);
					return;
				}

				// if we free a miniblock from the
				// middle of the miniblock list;
				if (tmp_mb->start_address == address &&
					tmp_mb->prev->start_address + tmp_mb->prev->size ==
					address && tmp_mb->start_address + tmp_mb->size ==
					tmp_mb->next->start_address) {
					free_middle_bl(arena, address, tmp_b, tmp_mb);
					return;
				}

				tmp_mb = tmp_mb->next;
				if (tmp_mb == tmp_b->miniblock_list->head)
					break;
			}
		}
		tmp_b = tmp_b->next;
		if (tmp_b == arena->block_list->head)
			break;
	}

	// if none of the functions above were called,
	// this means the address is invalid;
	printf("Invalid address for free.\n");
}

// function for READ command;
void read(arena_t *arena, unsigned long address, unsigned long size)
{
	if (arena->miniblocks == 0) {
		printf("Invalid address for read.\n");
		return;
	}
	short ok = 0;
	// for optimisation, first, we go through the arena, to verify
	// the permission and the correctness of the input adress;
	block_t *tmp_b = arena->block_list->head;
	while (1) {
		miniblock_t *tmp_mb = tmp_b->miniblock_list->head;
		while (1) {
			// we verify if we are allowed to read on
			// the requested adress;
			if (tmp_b->start_address == address) {
				if (tmp_mb->perm == 3 ||
					tmp_mb->perm == 2 ||
					tmp_mb->perm == 1 ||
					tmp_mb->perm == 0) {
					printf("Invalid permissions for read.\n");
					return;
				}
			}
			// this if verify that the address is valid;
			if (tmp_b->start_address <= address &&
				tmp_b->start_address + tmp_b->size >= address)
				ok = 1;

			tmp_mb = tmp_mb->next;
			if (tmp_mb == tmp_b->miniblock_list->head)
				break;
		}
		tmp_b = tmp_b->next;
		if (tmp_b == arena->block_list->head)
			break;
	}
	// if the adress is invalid, we return;
	if (!ok) {
		printf("Invalid address for read.\n");
		return;
	}
	ok = 0;
	tmp_b = arena->block_list->head;

	// now we know that the address is valid and we can read information,
	// so we are going through(again) the arena to printf the information;
	while (1) {
		miniblock_t *tmp_mb = tmp_b->miniblock_list->head;
		while (1) {
			if (tmp_b->start_address <= address &&
				tmp_b->start_address + tmp_b->size >= address) {
				if (tmp_b->size < size) {
					size = tmp_b->size;
					printf("Warning: size was bigger than the block size.");
					printf(" Reading %ld characters.\n", size);
				}
				for (int i = 0; i < tmp_mb->size; i++) {
					if (i + address - tmp_b->start_address >=
						tmp_mb->buffer_size ||
						i + address - tmp_b->start_address > size)
						break;
					// we are printing the characters one by one
					// because a part of a miniblock might be empty
					printf("%c", *((char *)tmp_mb->rw_buffer + address -
						   tmp_b->start_address + i));
				}
				ok = 1;
			}
			tmp_mb = tmp_mb->next;
			if (tmp_mb == tmp_b->miniblock_list->head)
				break;
		}
		tmp_b = tmp_b->next;
		if (tmp_b == arena->block_list->head)
			break;
	}
	if (ok) {
		printf("\n");
		return;
	}
}

// function for write command
void write(arena_t *arena, const unsigned long address,
		   unsigned long size, int8_t *data)
{
	if (arena->miniblocks == 0) {
		printf("Invalid address for write.\n");
		return;
	}
	int ok = 0;
	// for optimisation, first, we go through the arena, to verify
	// the permission and the correctness of the input adress;
	block_t *tmp_b = arena->block_list->head;
	while (1) {
		miniblock_t *tmp_mb = tmp_b->miniblock_list->head;
		while (1) {
			// we verify if we are allowed to write
			// on the requested adress;
			if (tmp_b->start_address == address) {
				if (tmp_mb->perm == 5 ||
					tmp_mb->perm == 4 ||
					tmp_mb->perm == 1 ||
					tmp_mb->perm == 0) {
					printf("Invalid permissions for write.\n");
					return;
				}
			}
			// this if verify that the address is valid;
			if (tmp_b->start_address <= address &&
				tmp_b->start_address + tmp_b->size >= address)
				ok = 1;

			tmp_mb = tmp_mb->next;
			if (tmp_mb == tmp_b->miniblock_list->head)
				break;
		}
		tmp_b = tmp_b->next;
		if (tmp_b == arena->block_list->head)
			break;
	}
	// if address in invalid we return;
	if (!ok) {
		printf("Invalid address for write.\n");
		return;
	}

	tmp_b = arena->block_list->head;
	while (1) {
		miniblock_t *tmp_mb = tmp_b->miniblock_list->head;
		while (1) {
			if (tmp_b->start_address <= address &&
				tmp_b->start_address + tmp_b->size >= address) {
				if (tmp_b->size < size ||
					address + size > tmp_b->start_address + tmp_b->size) {
					size = tmp_mb->start_address + tmp_mb->size - address;
					printf("Warning: size was bigger than the block size.");
					printf(" Writing %ld characters.\n", size);
				}
				tmp_mb->rw_buffer = malloc(tmp_mb->size);
				DIE(!tmp_mb->rw_buffer, "Malloc failed\n");

				tmp_mb->buffer_size = size;
				// we are writing the characters one by one
				// because a part of a miniblock might be empty
				for (int i = 0; i < tmp_mb->size; i++) {
					if (i >= size || i == tmp_mb->start_address
						+ tmp_mb->size - address)
						break;
					*((char *)tmp_mb->rw_buffer + address -
					tmp_b->start_address + i) = *((char *)data + i);
				}
				data = data + tmp_mb->size;
				ok = 1;
			}
			tmp_mb = tmp_mb->next;
			if (tmp_mb == tmp_b->miniblock_list->head)
				break;
		}
		tmp_b = tmp_b->next;
		if (tmp_b == arena->block_list->head)
			break;
	}
}

// funtion for PMAP command;
void pmap(const arena_t *arena)
{
	if (arena->size == 0) {
		printf("Total memory: 0x%lX bytes\n", arena->capacity);
		printf("Free memory: 0x%lX bytes\n", arena->capacity - arena->size);
		printf("Number of allocated blocks: %ld\n", arena->blocks);
		printf("Number of allocated miniblocks: %ld\n", arena->miniblocks);
		return;
	}
	int i = 1;
	block_t *tmp_b = arena->block_list->head;
	printf("Total memory: 0x%lX bytes\n", arena->capacity);
	printf("Free memory: 0x%lX bytes\n", arena->capacity - arena->size);
	printf("Number of allocated blocks: %ld\n", arena->blocks);
	printf("Number of allocated miniblocks: %ld\n", arena->miniblocks);
	printf("\n");

	// we go through the arena and printf everything;
	do {
		printf("Block %d begin\n", i);
		printf("Zone: 0x%lX - 0x%lX\n", tmp_b->start_address,
			   tmp_b->start_address + tmp_b->size);
		int j = 1;
		miniblock_t *tmp_mb = tmp_b->miniblock_list->head;
		do {
			printf("Miniblock %d:\t\t0x%lX\t\t-\t\t0x%lX\t\t| ", j,
				   tmp_mb->start_address, tmp_mb->start_address + tmp_mb->size);

			if (tmp_mb->perm == 7)
				printf("RWX");
			else if (tmp_mb->perm == 6)
				printf("RW-");
			else if (tmp_mb->perm == 5)
				printf("R-X");
			else if (tmp_mb->perm == 4)
				printf("R--");
			else if (tmp_mb->perm == 3)
				printf("-WX");
			else if (tmp_mb->perm == 2)
				printf("-W-");
			else if (tmp_mb->perm == 1)
				printf("--X");
			else if (tmp_mb->perm == 0)
				printf("---");
			printf("\n");

			tmp_mb = tmp_mb->next;
			++j;
			if (tmp_mb == tmp_b->miniblock_list->head)
				break;
		} while (1);
		printf("Block %d end\n", i);

		tmp_b = tmp_b->next;
		++i;
		if (tmp_b == arena->block_list->head)
			break;

		printf("\n");
	} while (1);
}

// function for MPROTECT command;
void mprotect(arena_t *arena, unsigned long address, uint8_t permission)
{
	// we go through all the arena
	// this function ends when we find the miniblock
	// where permission changes need to be made;
	block_t *tmp_b = arena->block_list->head;
	while (1) {
		miniblock_t *tmp_mb = tmp_b->miniblock_list->head;
		while (1) {
			if (tmp_mb->start_address == address) {
				tmp_mb->perm = permission;
				return;
			}
			tmp_mb = tmp_mb->next;
			if (tmp_mb == tmp_b->miniblock_list->head)
				break;
		}
		tmp_b = tmp_b->next;
		if (tmp_b == arena->block_list->head)
			break;
	}
	printf("Invalid address for mprotect.\n");
}
