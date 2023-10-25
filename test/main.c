#include "vma.h"

int main(void)
{
	arena_t *kernel_buffer;

	char command[MAX_STRING_SIZE];

	while (1) {
		scanf("%s", command);

		if (strncmp(command, "ALLOC_ARENA", 11) == 0) {
			unsigned long size_arena;
			scanf("%ld", &size_arena);

			kernel_buffer = alloc_arena(size_arena);
		} else if (strcmp(command, "DEALLOC_ARENA") == 0) {
			dealloc_arena(kernel_buffer);
			break;
		} else if (strncmp(command, "ALLOC_BLOCK", 11) == 0) {
			unsigned long start_address, block_size;
			scanf("%ld%ld", &start_address, &block_size);

			alloc_block(kernel_buffer, start_address, block_size);
		} else if (strcmp(command, "FREE_BLOCK") == 0) {
			unsigned long start_address;
			scanf("%ld", &start_address);

			free_block(kernel_buffer, start_address);
		} else if (strcmp(command, "READ") == 0) {
			unsigned long start_address, block_size;
			scanf("%ld%ld", &start_address, &block_size);

			read(kernel_buffer, start_address, block_size);
		} else if (strcmp(command, "WRITE") == 0) {
			unsigned long start_address, size;
			scanf("%ld%ld", &start_address, &size);
			void *data = malloc(size);
			char c;
			scanf("%c", &c);
			for (int i = 0; i < size; i++) {
				scanf("%c", &c);
				if (i == 0 && c == '\n')
					continue;
				*((char *)data + i) = c;
			}
			write(kernel_buffer, start_address, size, data);
			free(data);

		} else if (strcmp(command, "PMAP") == 0) {
			pmap(kernel_buffer);
		} else if (strncmp(command, "MPROTECT", 8) == 0) {
			unsigned long arena_address;
			scanf("%ld", &arena_address);
			char linie[50], permission[11], *p;
			unsigned char new_perm = 0;
			short int i = 0;
			fgets(linie, 49, stdin);
			p = strtok(linie, " ");
			while (p) {
				if (strncmp(p, "PROT_NONE", 9) == 0)
					new_perm = 0;
				if (strncmp(p, "PROT_READ", 9) == 0)
					new_perm = new_perm + 4;
				if (strncmp(p, "PROT_WRITE", 10) == 0)
					new_perm = new_perm + 2;
				if (strncmp(p, "PROT_EXEC", 9) == 0)
					new_perm = new_perm + 1;
				p = strtok(NULL, " ");
			}
			mprotect(kernel_buffer, arena_address, new_perm);
		} else {
			printf("Invalid command. Please try again.\n");
		}
	}
	return 0;
}
