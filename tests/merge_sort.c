#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <time.h>
#include <inttypes.h>

#include "../allocator.h"

/*
 * This implementation of merge sort uses many calls to malloc (which is slow)
 * for testing purposes.
 */
void
merge(int A[], size_t p, size_t q, size_t r)
{
	size_t n1 = q - p + 1, n2 = r - q, i = 0, j = 0, k = 0;
	int *L, *R;

	L = m_malloc(n1 * sizeof(int));
	R = m_malloc(n2 * sizeof(int));

	if (!L || !R) {
		fprintf(stderr, "Could not allocate memory.\n");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < n1; i++)
		L[i] = A[p + i];
	for (j = 0; j < n2; j++)
		R[j] = A[q + j + 1];

	i = 0; j = 0; k = p;
	while (i < n1 && j < n2) {
		if (L[i] <= R[j]) {
			A[k] = L[i];
			i++;
		} else {
			A[k] = R[j];
			j++;
		}
		k++;
	}

	while (i < n1) {
		A[k] = L[i];
		i++; k++;
	}
	while (j < n2) {
		A[k] = R[j];
		j++; k++;
	}

	m_free(L);
	m_free(R);
}

void
merge_sort(int A[], size_t p, size_t r)
{
	size_t q;

	if (p < r) {
		q = p + (r - p) / 2;
		merge_sort(A, p, q);
		merge_sort(A, q + 1, r);
		merge(A, p, q, r);
	}
}

int
main(int argc, char **argv)
{
	size_t i, size;
	int *A;

	if (argc < 2) {
		puts("No size provided.");
		return EXIT_SUCCESS;
	}

	for (i = 0; i < strlen(argv[1]); i++) {
		if (argv[1][i] < '0' || argv[1][i] > '9') {
			printf("%s is not a valid amount.\n", argv[1]);
			return EXIT_SUCCESS;
		}
	}
	errno = 0;
	size = (size_t) strtoumax(argv[1], NULL, 10);
	if (!size || errno == ERANGE) {
		printf("%s is not a valid amount.\n", argv[1]);
		return EXIT_SUCCESS;
	}

	A = m_malloc(size * sizeof(int));
	if (!A) {
		fprintf(stderr, "Could not allocate memory.\n");
		return EXIT_FAILURE;
	}
	srand(time(0));
	for (i = 0; i < size; i++)
		A[i] = rand();

	merge_sort(A, 0, size - 1);
	for (i = 0; i < size; i++)
		printf("A[%zu] = %d\n", i, A[i]);
	m_free(A);

	return EXIT_SUCCESS;
}
