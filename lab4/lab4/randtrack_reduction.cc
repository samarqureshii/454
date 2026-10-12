// randtrack_reduction.cc
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include "defs.h"
#include "hash.h"
#include "list.h"

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
    sample(unsigned k) : my_key(k), next(nullptr), count(0) {}
    unsigned key() { return my_key; }
    void print(FILE*) { printf("%u %u\n", my_key, count); }
};

static int sampling_interval;
static int num_threads;

struct TL { hash<sample,unsigned> table; };
static std::vector<TL*> tls;

hash<sample, unsigned> h; // final merged

struct Work { int begin, end; };
static std::vector<Work> work;

static void* worker(void* arg) {
    long tid = (long)arg;
    hash<sample,unsigned>& ht = tls[tid]->table;
    unsigned rnum;

    for (int s = work[tid].begin; s < work[tid].end; ++s) {
        rnum = (unsigned)s;
        for (int j = 0; j < SAMPLES_PER_STREAM; ++j) {
            unsigned num = 0;

            for (int k = 0; k < sampling_interval; ++k){
                num = rand_r(&rnum);
            }

            unsigned key = num % RAND_NUM_UPPER_BOUND;

            sample* n = ht.lookup(key);

            if (!n) { 
                n = new sample(key); ht.insert(n);
            }
            
            n->count++;
        }
    }
    return NULL;
}

static void merge_all() {
    h.setup(14);
    for (TL* t : tls) {
        hash<sample,unsigned>& src = t->table;
        const unsigned NB = src.size();
        for (unsigned b = 0; b < NB; ++b) {
            list<sample,unsigned>* lst = src.get_list(b);

            for (sample* e = lst->head(); e; e = e->next) {
                unsigned key = e->key();
                sample* g = h.lookup(key);

                if (!g) {
                    g = new sample(key); h.insert(g); 
                }

                g->count += e->count;
            }
        }
    }
}

static void usage(const char* p) {
    printf("Usage: %s <num_threads:1|2|4|8> <sampling_interval> \n", p);
    exit(1);
}

int main(int argc, char** argv) {
    if (argc != 3) usage(argv[0]);
    sscanf(argv[1], "%d", &num_threads);
    sscanf(argv[2], "%d", &sampling_interval);
    if ((num_threads!=1 && num_threads!=2 && num_threads!=4 && num_threads!=8) || sampling_interval<=0)
        usage(argv[0]);

    printf("Team Name: %s\n\n", team.team);
    printf("Student Name: %s\n", team.name);
    printf("Student Student Number: %s\n", team.number);
    printf("Student Email: %s\n\n", team.email);

    tls.resize(num_threads, nullptr);
    for (int t=0;t<num_threads;++t) { 
        tls[t] = new TL(); tls[t]->table.setup(14); 
    }

    work.resize(num_threads);
    int per = NUM_SEED_STREAMS/num_threads, rem = NUM_SEED_STREAMS%num_threads, cur=0;
    for (int t=0;t<num_threads;++t){ 
        int take=per+(t<rem); 
        work[t]={cur,cur+take}; 
        cur+=take; 
        }

    std::vector<pthread_t> tids(num_threads);
    for (long t=0;t<num_threads;++t){
        pthread_create(&tids[t], nullptr, worker, (void*)t);
    }

    for (int t=0;t<num_threads;++t){
        pthread_join(tids[t], nullptr);
    }

    merge_all();
    h.print();
    return 0;
}
