#define _GNU_SOURCE

#include "life.h"
#include "util.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>

#define WORD_BITS 64
#define MAX_N 10000

// words per row at maximum N 
#define MAX_WORDS_PER_ROW ((MAX_N + WORD_BITS - 1) / WORD_BITS)
#define MAX_WORDS         (MAX_N * MAX_WORDS_PER_ROW)

// 256MB global bitboards, aligned to cache-line boundary to reduce false sharing
static uint64_t g_bit_cur[MAX_WORDS]  __attribute__((aligned(64)));
static uint64_t g_bit_next[MAX_WORDS] __attribute__((aligned(64)));

//og char layout 
#define BOARD(__board, __i, __j, __LDA)  (__board)[(__i) + (__LDA) * (__j)]

#define CHECK_INTERVAL 100
#define MAX_HISTORY    512

static inline uint64_t
mix64(uint64_t x)
{
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;
    return x;
}

//hash current bitboard state
static uint64_t
hashBitboard(const uint64_t *bit_cur,
             int nrows, int ncols, int words_per_row)
{
    (void)ncols; //remove

    uint64_t h = 1469598103934665603ULL; //random seed
    const int total_words = nrows * words_per_row;

    for (int w = 0; w < total_words; w++) {
        uint64_t x = bit_cur[w];
        h ^= mix64(x);
    }
    return h;
}

/*****************************************************************************
 * bitboard helpers*/

static inline int
bitboard_get(const uint64_t *bitboard,
             int words_per_row,
             int row, int col)
{ //get bit at row,col from bitboard, assume 0<=row<nrows and 0<=col<ncols
    int word_idx = row * words_per_row + (col >> 6);  /* col / 64 */
    int bit_idx  = col & 63;                          /* col % 64 */
    return (int)((bitboard[word_idx] >> bit_idx) & 1ULL);
}

//set bit at (row, col) to val (0 or 1) 
static inline void
setBitboard(uint64_t *bitboard,
            int words_per_row,
            int row, int col,
            int val)
{
    int word_idx = row * words_per_row + (col >> 6);
    int bit_idx  = col & 63;
    uint64_t mask = 1ULL << bit_idx;
    if (val)
        bitboard[word_idx] |= mask;
    else
        bitboard[word_idx] &= ~mask;
}

static void
packBoard(const char *board,
          int nrows, int ncols, int LDA,
          uint64_t *bit_cur,
          int words_per_row,
          int *any_active_out)
{
    const int total_words = nrows * words_per_row;
    for (int w = 0; w < total_words; w++) {
        bit_cur[w] = 0ULL;
    }

    int any_active = 0;

    for (int i = 0; i < nrows; i++) {
        for (int j = 0; j < ncols; j++) {
            int val = BOARD(board, i, j, LDA) ? 1 : 0;
            if (val) {
                setBitboard(bit_cur, words_per_row, i, j, 1);
                any_active = 1;
            }
        }
    }

    *any_active_out = any_active;
}

static void
unpackBitboardToBoard(char *board,
                      int nrows, int ncols, int LDA,
                      const uint64_t *bit_cur,
                      int words_per_row)
{
    for (int i = 0; i < nrows; i++) {
        for (int j = 0; j < ncols; j++) {
            int val = bitboard_get(bit_cur, words_per_row, i, j);
            BOARD(board, i, j, LDA) = (char)val;
        }
    }
}

// for each word, compute 8 neighbour masks then make 4 bitplanes that represent neighbour count for each bit in that word
// then apply life rule next = (count == 3) | (alive & (count == 2))
static int
step_bitboard_row(const uint64_t *bit_cur,
                  uint64_t       *bit_next,
                  int nrows, int ncols,
                  int words_per_row,
                  int row_index)
{
    (void)ncols; //guaranteed multiple of 64, if not fallback later 

    const int W = words_per_row;

    int i  = row_index;
    int im1 = (i == 0)        ? (nrows - 1) : (i - 1);
    int ip1 = (i == nrows-1)  ? 0           : (i + 1);

    const uint64_t *rowA = bit_cur + im1 * W;
    const uint64_t *rowC = bit_cur + i   * W;
    const uint64_t *rowB = bit_cur + ip1 * W;

    uint64_t *rowN = bit_next + i * W;

    int row_active = 0;

    for (int w = 0; w < W; w++) {
        int prev = (w == 0)   ? (W - 1) : (w - 1);
        int next = (w == W-1) ? 0       : (w + 1);

        uint64_t a = rowA[w];
        uint64_t b = rowC[w];
        uint64_t c = rowB[w];

        // toroidal wrap across
        uint64_t aL = (a << 1) | (rowA[prev] >> 63);
        uint64_t aR = (a >> 1) | (rowA[next] << 63);

        uint64_t bL = (b << 1) | (rowC[prev] >> 63);
        uint64_t bR = (b >> 1) | (rowC[next] << 63);

        uint64_t cL = (c << 1) | (rowB[prev] >> 63);
        uint64_t cR = (c >> 1) | (rowB[next] << 63);

        // neighbour bitmasks for each word
        uint64_t N0 = aL;  /* NW */
        uint64_t N1 = a;   /* N  */
        uint64_t N2 = aR;  /* NE */
        uint64_t N3 = bL;  /* W  */
        uint64_t N4 = bR;  /* E  */
        uint64_t N5 = cL;  /* SW */
        uint64_t N6 = c;   /* S  */
        uint64_t N7 = cR;  /* SE */

        uint64_t c0 = 0ULL;
        uint64_t c1 = 0ULL;
        uint64_t c2 = 0ULL;
        uint64_t c3 = 0ULL;

#define ADD_NEIGHBOR(add)                        \
        do {                                     \
            uint64_t carry = (add);              \
            uint64_t s0 = c0 ^ carry;            \
            carry      = c0 & carry;             \
            c0 = s0;                             \
            uint64_t s1 = c1 ^ carry;            \
            carry      = c1 & carry;             \
            c1 = s1;                             \
            uint64_t s2 = c2 ^ carry;            \
            carry      = c2 & carry;             \
            c2 = s2;                             \
            uint64_t s3 = c3 ^ carry;            \
            carry      = c3 & carry;             \
            c3 = s3;                             \
        } while (0)

        ADD_NEIGHBOR(N0);
        ADD_NEIGHBOR(N1);
        ADD_NEIGHBOR(N2);
        ADD_NEIGHBOR(N3);
        ADD_NEIGHBOR(N4);
        ADD_NEIGHBOR(N5);
        ADD_NEIGHBOR(N6);
        ADD_NEIGHBOR(N7);

#undef ADD_NEIGHBOR

        // we only care about count == 2 or 3
        // https://www.jagregory.com/abrash-black-book/#chapter-17-the-game-of-life
        uint64_t not_c0 = ~c0;
        uint64_t not_c2 = ~c2;
        uint64_t not_c3 = ~c3;

        uint64_t eq2 = not_c3 & not_c2 &  c1    & not_c0;
        uint64_t eq3 = not_c3 & not_c2 &  c1    &  c0;

        uint64_t cur = b;

        /* Life rule: alive_next = (count == 3) | (alive & count == 2) */
        uint64_t next_word = eq3 | (cur & eq2);

        rowN[w] = next_word;

        if (next_word != 0ULL) {
            row_active = 1;
        }
    }

    return row_active;
}

char*
game_of_life(char *outboard, char *inboard,
             const int nrows, const int ncols,
             const int num_generations)
{
    // fix parallelism: exactly 16 threads, no dynamic resizing
    omp_set_dynamic(0);
    omp_set_num_threads(16);

    const int LDA = nrows;

    if (nrows < 32 || ncols < 32 || nrows > MAX_N || ncols > MAX_N) {
        exit(EXIT_FAILURE);
    }


    if ((ncols % WORD_BITS) != 0) { //ncols has to be a multiple of 64, if its not then have a fallback
        return sequential_game_of_life(outboard, inboard,
                                       nrows, ncols, num_generations);
    }

    const int words_per_row = ncols / WORD_BITS;
    const int total_words   = nrows * words_per_row;

    uint64_t *bit_cur  = g_bit_cur;
    uint64_t *bit_next = g_bit_next;

    int any_active_init = 0;
    packBoard(inboard, nrows, ncols, LDA, bit_cur, words_per_row, &any_active_init);

    if (!any_active_init) { //board is completely dead
        if (outboard != inboard) {
            memcpy(outboard, inboard, (size_t)nrows * (size_t)ncols);
            return outboard;
        }
        return inboard;
    }

    uint64_t history_hashes[MAX_HISTORY];
    int      history_gen[MAX_HISTORY];
    int      history_count = 0;

    int limit = num_generations;

    #pragma omp parallel
    {
        int tid    = omp_get_thread_num();
        int nprocs = sysconf(_SC_NPROCESSORS_ONLN);

        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);

        CPU_SET(tid % nprocs, &cpuset);

        pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    }

    for (int gen = 0; gen < limit; gen++) {
        int any_active_next = 0;

        //abrash single thread -> 0.81s, PR now ~0.15s for 16 threads on 8*2 phys cores
        #pragma omp parallel for schedule(static) reduction(|:any_active_next)
        for (int i = 0; i < nrows; i++) {
            int row_active = step_bitboard_row(bit_cur, bit_next,
                                               nrows, ncols,
                                               words_per_row,
                                               i);
            any_active_next |= row_active;
        }

        uint64_t *tmp = bit_cur; //swap curr and next bitboards
        bit_cur       = bit_next;
        bit_next      = tmp;

        if (!any_active_next) { //if all dead
            limit = gen + 1;
            break;
        }

        // CHECK_INTERVAL generations detection
        int current_gen = gen + 1;
        if (current_gen % CHECK_INTERVAL == 0) {
            uint64_t h = hashBitboard(bit_cur, nrows, ncols, words_per_row);

            int found = -1;
            for (int k = 0; k < history_count; k++) {
                if (history_hashes[k] == h) {
                    found = k;
                    break;
                }
            }

            if (found >= 0) {
                int g1 = history_gen[found];
                int g2 = current_gen;
                int cycle_len = g2 - g1;

                int remaining = num_generations - current_gen;

                if (cycle_len > 0 && remaining > 0) {
                    int remainder = remaining % cycle_len;
                    limit = current_gen + remainder; //dont loop past gen limit
                }
            } else {
                if (history_count < MAX_HISTORY) { //if FIFO not full then store this hash
                    history_hashes[history_count] = h;
                    history_gen[history_count]    = current_gen;
                    history_count++;
                } else { //shift left and append
                    for (int k = 1; k < MAX_HISTORY; k++) {
                        history_hashes[k - 1] = history_hashes[k];
                        history_gen[k - 1]    = history_gen[k];
                    }
                    history_hashes[MAX_HISTORY - 1] = h;
                    history_gen[MAX_HISTORY - 1]    = current_gen;
                }
            }
        }
    } /* gen loop */

    unpackBitboardToBoard(outboard, nrows, ncols, LDA,
                          bit_cur, words_per_row);

    return outboard;
}
