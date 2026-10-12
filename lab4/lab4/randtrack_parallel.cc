// randtrack_unsafe.cc  — parallelized with pthreads (NO synchronization)
// Purpose: step 1 skeleton to evolve into the 4 sync variants.

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include "defs.h"
#include "hash.h"

#define SAMPLES_PER_STREAM      10000000
#define RAND_NUM_UPPER_BOUND      100000
#define NUM_SEED_STREAMS                8

team_t team = {
    "team summer",
    "Samar Qureshi",
    "1009149583",
    "ssamar.qureshi@mail.utoronto.ca",
};

class sample {
    unsigned my_key;
public:
    sample *next;
    unsigned count;

    sample(unsigned the_key) : my_key(the_key), next(nullptr), count(0) {}
    unsigned key() { return my_key; }
    void print(FILE * /*f*/) { printf("%u %u\n", my_key, count); }
};

//global hash table
hash<sample, unsigned> h;

static int sampling_interval;
static int num_threads;

struct Work { int begin, end; };
static std::vector<Work> work;

static void* worker(void* arg) {
    long tid = (long)arg;
    unsigned rnum;

    for (int stream = work[tid].begin; stream < work[tid].end; ++stream) {
        rnum = (unsigned)stream;

        for (int j = 0; j < SAMPLES_PER_STREAM; ++j) {
            unsigned num = 0;
            for (int k = 0; k < sampling_interval; ++k)
                num = rand_r(&rnum);

            unsigned key = num % RAND_NUM_UPPER_BOUND;

			// critical section without synchronization
            sample *s = h.lookup(key);
            if (!s) {
                s = new sample(key);
                h.insert(s);
            }
            s->count++;
        }
    }
    return nullptr;
}

static void usage(const char* prog) {
    printf("Usage: %s <num_threads:1|2|4|8> <sampling_interval> \n", prog);
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc != 3) usage(argv[0]);
    if (sscanf(argv[1], "%d", &num_threads) != 1 ||
        sscanf(argv[2], "%d", &sampling_interval) != 1) usage(argv[0]);

    if ((num_threads != 1 && num_threads != 2 && num_threads != 4 && num_threads != 8) ||
        sampling_interval <= 0) usage(argv[0]);

    printf("Team Name: %s\n\n", team.team);
    printf("Student Name: %s\n", team.name);
    printf("Student Student Number: %s\n", team.number);
    printf("Student Email: %s\n\n", team.email);

    // init 16K buckets
    h.setup(14);

    work.resize(num_threads);
    int per = NUM_SEED_STREAMS / num_threads;
    int rem = NUM_SEED_STREAMS % num_threads;
    int cur = 0;
    for (int t = 0; t < num_threads; ++t) {
        int take = per + ((t < rem) ? 1 : 0);
        work[t] = Work{cur, cur + take};
        cur += take;
    }

    std::vector<pthread_t> tids(num_threads);
    for (long t = 0; t < num_threads; ++t)
        pthread_create(&tids[t], nullptr, worker, (void*)t);
    for (int t = 0; t < num_threads; ++t)
        pthread_join(tids[t], nullptr);

    h.print();
    return 0;
}
