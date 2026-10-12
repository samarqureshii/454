#include "load.h"
#include <assert.h>
#include <stdlib.h>
#define MIN_N 32
#define MAX_N 10000


//char *
// make_board(const int nrows, const int ncols)
// {
// 	char *board = NULL;
// 	int i;

// 	/* Allocate the board and fill in with 'Z' instead of a number, so
// 	   that it's easy to diagnose bugs */
// 	board = malloc(nrows * ncols * sizeof(char));
// 	assert(board != NULL);
// 	for (i = 0; i < nrows * ncols; i++)
// 		board[i] = 'Z';

// 	return board;
// }
char* make_board(const int nrows, const int ncols)
{
    char *board = malloc((size_t)nrows * (size_t)ncols);
    if (!board) {
        fprintf(stderr, "make_board: malloc failed\n");
        exit(EXIT_FAILURE);
    }
    return board;
}


static void load_dimensions(FILE * input, int *nrows, int *ncols)
{
	int ngotten = 0;

	ngotten = fscanf(input, "P1\n%d %d\n", nrows, ncols);
	if (ngotten < 1) {
		fprintf(stderr,"failed to read 'P1' and board dimensions\n");
		fclose(input);
		exit(EXIT_FAILURE);
	}
	if (*nrows < 1) {
		fprintf(stderr, "number of rows %d must be positive\n", *nrows);
		fclose(input);
		exit(EXIT_FAILURE);
	}
	if (*ncols < 1) {
		fprintf(stderr, "number of cols %d must be positive\n", *ncols);
		fclose(input);
		exit(EXIT_FAILURE);
	}

    if (*nrows > MAX_N || *ncols > MAX_N) {
        // fprintf(stderr,
        //         "image size %d x %d is too large; maximum is %d x %d\n",
        //         *nrows, *ncols, MAX_N, MAX_N);
        fclose(input);
        exit(EXIT_FAILURE);
    }

}

// static char *
// load_board_values(FILE * input, const int nrows, const int ncols)
// {
// 	char *board = NULL;
// 	int ngotten = 0;
// 	int i = 0;

// 	/* Make a new board */
// 	board = make_board(nrows, ncols);

// 	/* Fill in the board with values from the input file */
// 	for (i = 0; i < nrows * ncols; i++) {
// 		ngotten = fscanf(input, "%c\n", &board[i]);
// 		if (ngotten < 1) {
// 			fprintf(stderr, "ran out of input at item %d\n", i);
// 			fclose(input);
// 			exit(EXIT_FAILURE);
// 		} else {
// 			/* ASCII '0' is not zero; do the conversion */
// 			board[i] = board[i] - '0';
// 		}
// 	}

// 	return board;
// }

static char *
load_board_values(FILE * input, const int nrows, const int ncols)
{
    char *board = make_board(nrows, ncols);
    const int total = nrows * ncols;

    for (int i = 0; i < total; i++) {
        int ch;

        /* skip whitespace (spaces, tabs, newlines, etc.) */
        do {
            ch = fgetc(input);
            if (ch == EOF) {
                fprintf(stderr, "ran out of input at item %d\n", i);
                fclose(input);
                exit(EXIT_FAILURE);
            }
        } while (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n');

        /* Now ch should be '0' or '1' */
        board[i] = (char)(ch - '0');
    }

    return board;
}


char *
load_board(FILE * input, int *nrows, int *ncols)
{
	load_dimensions(input, nrows, ncols);
	return load_board_values(input, *nrows, *ncols);
}
