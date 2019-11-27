#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

#define SIZE 1024

volatile __uint64_t A[SIZE][SIZE];
volatile __uint64_t B[SIZE][SIZE];
volatile __uint64_t C[SIZE][SIZE];
volatile __uint64_t D[SIZE][SIZE];

void init(volatile __uint64_t A[][SIZE], volatile __uint64_t B[][SIZE])
{
	int r, c;

	for (c = 0; c < SIZE; c++) {
		for (r = 0; r < SIZE; r++) {
			A[r][c] = rand();
			B[r][c] = rand();
		}
	}
}

int verify(volatile __uint64_t C[][SIZE], volatile __uint64_t D[][SIZE])
{
	int r, c;

	for (c = 0; c < SIZE; c++) {
		for (r = 0; r < SIZE; r++) {
			if (C[r][c] != D [r][c]) {
				printf("%ld =? %ld\n", C[r][c], D[r][c]);
				printf("error!\n");
				return -1;
			}
			
		}
	}
	return 0;
}

void transpose()
{
	// Method: Transpose
	// Input: An n x m matrix
	// Output: Matrix with newCols = oldRows
	int row, col;
	u_int64_t tmp;
	for (row = 0; row < SIZE; row++) {
		for (col = 0; col < SIZE; col++) {
			tmp = B[col][row];
			B[col][row] = B[row][col];
			B[row][col] = tmp;
		}
	}
}

void transposeMatmul(volatile __uint64_t A[][SIZE], volatile __uint64_t B[][SIZE])
{
	int rowA, colB, idx;
	for (rowA = 0; rowA < SIZE; rowA++) {
		for (colB = 0; colB < SIZE; colB++) {
			for (idx = 0; idx < SIZE; idx++) {
				C[rowA][colB] += A[rowA][idx] * B[colB][idx];
			}
		}
	}
}

void matmul(volatile __uint64_t A[][SIZE], volatile __uint64_t B[][SIZE])
{
	// Added ctr to count how many matmul ops are executed
	int rowA, colB, idx;
//	int ctr = 0;
	for (rowA = 0; rowA < SIZE; rowA++) {
		for (colB = 0; colB < SIZE; colB++) {
			for (idx = 0; idx < SIZE; idx++) {
//				ctr++;
				C[rowA][colB] += A[rowA][idx] * B[idx][colB];
			}
		}
	}
//	printf("Num Ops: %d\n", ctr);
}

void tile(volatile __uint64_t A[][SIZE], volatile __uint64_t B[][SIZE], volatile __uint64_t T[][SIZE], int blckSize) 
{
	// Loop over the entire N x M matrix
	for (int rowA = 0; rowA < SIZE; rowA += blckSize) {
		for (int colB = 0; colB < SIZE; colB += blckSize) {
			for (int idx = 0; idx < SIZE; idx += blckSize) {
				// Loop over sub-blocks of the N x M matrix
				for (int rowA2 = rowA; rowA2 < rowA + blckSize && rowA2 < SIZE; rowA2++) {
					for (int colB2 = colB; colB2 < colB + blckSize && colB2 < SIZE; colB2++) {
						for (int idx2 = idx; idx2 < idx + blckSize && idx2 < SIZE; idx2++) {
							T[rowA2][colB2] += A[rowA2][idx2] * B[idx2][colB2];
						}
					}
				}
			}
		}
	}	
	
}

void transpose_tile(volatile __uint64_t A[][SIZE], volatile __uint64_t B[][SIZE], volatile __uint64_t T[][SIZE], int blckSize) 
{
	// Loop over the entire N x M matrix
	for (int rowA = 0; rowA < SIZE; rowA += blckSize) {
		for (int colB = 0; colB < SIZE; colB += blckSize) {
			for (int idx = 0; idx < SIZE; idx += blckSize) {
				// Loop over sub-blocks of the N x M matrix
				for (int rowA2 = rowA; rowA2 < rowA + blckSize && rowA2 < SIZE; rowA2++) {
					for (int colB2 = colB; colB2 < colB + blckSize && colB2 < SIZE; colB2++) {
						for (int idx2 = idx; idx2 < idx + blckSize && idx2 < SIZE; idx2++) {
							T[rowA2][colB2] += A[rowA2][idx2] * B[colB2][idx2];
						}
					}
				}
			}
		}
	}	
	
}

int main(int argc, char **argv)
{
	clock_t t, d;
	double time_taken;

	init(A, B);
	memset((__uint64_t**)C, 0, sizeof(__uint64_t) * SIZE * SIZE);
	memset((__uint64_t**)D, 0, sizeof(__uint64_t) * SIZE * SIZE);

	printf("#### Testing Start ####\n\n");
	
	printf("#### Testing Tile Matmul (isTranspose = False) #### \n");
	for (int i = 1; i <= 1024; ) {
		memset((__uint64_t**)D, 0, sizeof(__uint64_t) * SIZE * SIZE);
		d = clock();
		tile(A, B, D, i);
		d = clock() - d;
		time_taken = ((double)d)/CLOCKS_PER_SEC; // in seconds
		printf("Tile Matmul for %d took %f seconds to execute \n", i, time_taken);
		i = i << 1;
	}

	printf("\n#### Testing Naive Matmul (isTranspose = False) #### \n");
	t = clock();
	matmul(A, B);
	t = clock() - t;
	time_taken = ((double)t)/CLOCKS_PER_SEC; // in seconds
	printf("Naive Matmul took %f seconds to execute \n", time_taken);
	
	printf("\n#### Testing Correctness of Tiling Algorithm #### \n");
	printf("Correctness of Tiling Algorithm (if not 0 then incorrect): %d \n", verify(C, D));

	// Reset C Matrix
	memset((__uint64_t**)C, 0, sizeof(__uint64_t) * SIZE * SIZE);

	printf("\n#### Testing Matmul (isTranspose = True) #### \n");
	transpose();
	t = clock();
	transposeMatmul(A, B);
	t = clock() - t;
	time_taken = ((double)t)/CLOCKS_PER_SEC; // in seconds
	printf("Transpose Matmul took %f seconds to execute \n", time_taken);

	printf("\n#### Testing Tile Matmul (isTranspose = True) #### \n");
	for (int i = 1; i <= 1024; ) {
		memset((__uint64_t**)D, 0, sizeof(__uint64_t) * SIZE * SIZE);
		d = clock();
		transpose_tile(A, B, D, i);
		d = clock() - d;
		time_taken = ((double)d)/CLOCKS_PER_SEC; // in seconds
		printf("Transpose Tile Matmul for %d took %f seconds to execute \n", i, time_taken);
		i = i << 1;
	}

	printf("\n#### Testing Correctness of Tiling Algorithm #### \n");
	printf("Correctness of Tiling Algorithm (if not 0 then incorrect): %d \n", verify(C, D));

	printf("\n#### Testing Complete #### \n");
}
