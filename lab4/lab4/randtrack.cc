#include <stdio.h>
#include <stdlib.h>

#include "defs.h"
#include "hash.h"

#define SAMPLES_PER_STREAM   10000000
#define RAND_NUM_UPPER_BOUND   100000
#define NUM_SEED_STREAMS            8

/* 
 * ECE454 Students: 
 * Please fill in the following team struct 
 */
team_t team = {
	"team summer",	/* Team name */

	"Samar Qureshi",	/* Member full name */
	"1009149583",	/* Member student number */
	"ssamar.qureshi@mail.utoronto.ca",	/* Member email address */
};

class sample;

class sample {
	unsigned my_key;
public:
	sample *next;
	unsigned count;

	sample(unsigned the_key) {
		my_key = the_key;
		count = 0;
	};
	unsigned key() {
		return my_key;
	}
	void print(FILE * f) {
		printf("%d %d\n", my_key, count);
	}
};

/* The code below uses a C++ template to instantiate an empty hash table. */
/* The hash table implementation takes two types, an element type and a key
 * type. We instantiate the element type as the sample class (defined above) and
 * the key type is instantiated to be unsigned. */
hash<sample, unsigned> h;

void
usage(char *prog)
{
	printf("Usage: %s <num_threads> <sampling_interval>\n", prog);
	printf("       num_threads should be 1, 2, 4 or 8\n");
	printf("       sampling_interval should be > 0\n");
	exit(1);
}

static int sampling_interval;
static int num_threads;

int
main(int argc, char *argv[]) {
	/* Parse program arguments */
	if (argc != 3) {
		usage(argv[0]);
	}
	/* num_threads is not used in this single-threaded version */
	sscanf(argv[1], " %d", &num_threads);
	sscanf(argv[2], " %d", &sampling_interval);

	if (num_threads != 1 && num_threads != 2 && num_threads != 4 &&
	    num_threads != 8) {
		usage(argv[0]);
	}
	if (sampling_interval <= 0) {
		usage(argv[0]);
	}

	// Print out team information
	printf("Team Name: %s\n", team.team);
	printf("\n");
	printf("Student Name: %s\n", team.name);
	printf("Student Student Number: %s\n", team.number);
	printf("Student Email: %s\n", team.email);
	printf("\n");

	/* initialize a 16K-entry (2**14) hash of empty lists */
	h.setup(14);

	unsigned int rnum;
	/* process streams starting with different initial numbers */
	for (int i = 0; i < NUM_SEED_STREAMS; i++) {
		rnum = i;

		/* collect samples */
		for (int j = 0; j < SAMPLES_PER_STREAM; j++) {
			unsigned int num = 0;

			/* sample the numbers */
			for (int k = 0; k < sampling_interval; k++) {
				num = rand_r((unsigned int *)&rnum);
			}

			/* force the sample to be within the range of
			   0..RAND_NUM_UPPER_BOUND-1 */
			unsigned key = num % RAND_NUM_UPPER_BOUND;

			sample *s;
			/* if this sample has not been counted before */
			if (!(s = h.lookup(key))) {
				/* insert new element for it into hash table */
				s = new sample(key);
				h.insert(s);
			}
			/* increment the count for the sample */
			s->count++;
		}
	}

	/* print a list of the frequency of all samples */
	h.print();
	exit(0);
}
