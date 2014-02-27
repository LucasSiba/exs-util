#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>

#include "sxe-log.h"
#include "exs-pool.h"

/*
     | STATES_INFO * Number of states   |
     | ELEMS_INFO  * Number of elements |
     | POOL_INFO                        |
 0  --------------------------------------
     | Actual Data Array                |
     |----------------------------------|

     Newest                             Oldest
     head <=> prev-next <=> prev-next <=> tail 
 */

typedef struct STATES_INFO {
    unsigned head;
    unsigned tail;
    unsigned count;
} STATES_INFO;

typedef struct ELEMS_INFO {
    unsigned prev;
    unsigned next;
    unsigned state;
    struct   timeval state_time;
} ELEMS_INFO;

typedef struct POOL_INFO {
    const char * name;
    unsigned     num_states;
    size_t       sizeof_elem;
    unsigned     num_elems;
} POOL_INFO;

#define POOL_POOL_INFO   ((POOL_INFO*)  (pool - sizeof(POOL_INFO)))
#define POOL_ELEMS_INFO  ((ELEMS_INFO*) (pool - sizeof(POOL_INFO) - (sizeof(ELEMS_INFO) * POOL_POOL_INFO->num_elems)))
#define POOL_STATES_INFO ((STATES_INFO*)(pool - sizeof(POOL_INFO) - (sizeof(ELEMS_INFO) * POOL_POOL_INFO->num_elems) - (sizeof(STATES_INFO) * POOL_POOL_INFO->num_states)))

void
test_verify_pool_internals(void * array)
{
    char * pool = array;
    unsigned num_states = POOL_POOL_INFO->num_states;
    unsigned i;

    for (i = 0; i < num_states; i++) {
        if (POOL_STATES_INFO[i].head == POOL_NO_INDEX || POOL_STATES_INFO[i].tail == POOL_NO_INDEX) {
            SXEA1(POOL_STATES_INFO[i].head == POOL_NO_INDEX, "Head check - state=%u", i);
            SXEA1(POOL_STATES_INFO[i].tail == POOL_NO_INDEX, "Tail check - state=%u", i);
            SXEA1(POOL_STATES_INFO[i].count == 0, "Count check - state=%u", i);
        } else {
            SXEA1(POOL_STATES_INFO[i].count != 0, "State count != 0");
            SXEA1(POOL_ELEMS_INFO[POOL_STATES_INFO[i].tail].state == i, "Tail of state=%u, id=%u", i, POOL_STATES_INFO[i].tail);
            SXEA1(POOL_ELEMS_INFO[POOL_STATES_INFO[i].head].state == i, "Head of state=%u, id=%u", i, POOL_STATES_INFO[i].head);
            SXEA1(POOL_ELEMS_INFO[POOL_STATES_INFO[i].tail].next == POOL_NO_INDEX, "Tail Next - state=%u", i);
            SXEA1(POOL_ELEMS_INFO[POOL_STATES_INFO[i].head].prev == POOL_NO_INDEX, "Head Prev - state=%u", i);
        }

        unsigned j;
        unsigned id = POOL_STATES_INFO[i].head;
        unsigned prev = POOL_NO_INDEX;
        for (j = 1; j < POOL_STATES_INFO[i].count; j++) {
            SXEA1(id != POOL_NO_INDEX, "List walk head to tail != POOL_NO_INDEX");
            SXEA1(prev == POOL_ELEMS_INFO[id].prev, "List walk head to tail - Check prev's");
            prev = id;
            id = POOL_ELEMS_INFO[id].next;

        }
        id = POOL_STATES_INFO[i].tail;
        unsigned next = POOL_NO_INDEX;
        for (j = 1; j < POOL_STATES_INFO[i].count; j++) {
            SXEA1(id != POOL_NO_INDEX, "List walk tail to head != POOL_NO_INDEX");
            SXEA1(next == POOL_ELEMS_INFO[id].next, "List walk tail to head - Check next's");
            next = id;
            id = POOL_ELEMS_INFO[id].prev;
        }
    }
}

void *
exs_pool_new(const char * name, unsigned num_elems, size_t sizeof_elem, unsigned num_states)
{
    SXEL6("exs_pool_new(name=%s, num_elems=%u, sizeof_elem=%zu, num_states=%d)", name, num_elems, sizeof_elem, num_states);

    SXEA1(name != NULL, "Pool name isn't null");
    SXEA1(num_elems != 0, "num_elems is not zero");
    SXEA1(sizeof_elem != 0, "sizeof_elem is not zero");
    SXEA1(num_states != 0, "num_states is not zero");

    // Calculate total pool size
    unsigned pool_size = (
        sizeof(STATES_INFO) * num_states +
        sizeof(ELEMS_INFO) * num_elems + 
        sizeof(POOL_INFO) +
        sizeof_elem * num_elems
    );

    SXEL6("Allocated %u bytes for pool", pool_size);
    char * full_pool = malloc(pool_size);
    char * pool = &(full_pool[pool_size - (sizeof_elem * num_elems)]);

    POOL_POOL_INFO->name        = name;
    POOL_POOL_INFO->num_states  = num_states;
    POOL_POOL_INFO->sizeof_elem = sizeof_elem;
    POOL_POOL_INFO->num_elems   = num_elems;

    unsigned i;
    for (i = 0; i < num_states; i++) {
        if (i == 0) {
            POOL_STATES_INFO[i].head  = 0;
            POOL_STATES_INFO[i].tail  = num_elems - 1;
            POOL_STATES_INFO[i].count = num_elems;
        } else {
            POOL_STATES_INFO[i].head  = POOL_NO_INDEX;
            POOL_STATES_INFO[i].tail  = POOL_NO_INDEX;
            POOL_STATES_INFO[i].count = 0;
        }
    }
    for (i = 0; i < num_elems; i++) {
        if (i == 0) {
            POOL_ELEMS_INFO[i].prev = POOL_NO_INDEX;
            POOL_ELEMS_INFO[i].next = i + 1;
        } else if (i == num_elems - 1) {
            POOL_ELEMS_INFO[i].prev = i - 1;
            POOL_ELEMS_INFO[i].next = POOL_NO_INDEX;
        } else {
            POOL_ELEMS_INFO[i].prev = i - 1;
            POOL_ELEMS_INFO[i].next = i + 1;
        }
        POOL_ELEMS_INFO[i].state = 0;
        gettimeofday(&(POOL_ELEMS_INFO[i].state_time), NULL);
    }

    return pool;
}

void exs_pool_del(void * array) { char * pool = array; free(POOL_STATES_INFO); }

unsigned
exs_pool_get_number_in_state(void * array, unsigned state)
{
    char * pool = array;
    SXEL6("exs_pool_get_number_in_state(pool=%p, state=%u)", pool, state);
    SXEA1(array != NULL, "pool array is not null");
    SXEA1(state < POOL_POOL_INFO->num_states, "state (%u) is out of range (%u total states)", state, POOL_POOL_INFO->num_states);

    unsigned count = POOL_STATES_INFO[state].count;
    SXEL6("return %u", count);
    return count;
}

unsigned
exs_pool_index_to_state(void * array, unsigned id)
{
    char * pool = array;
    SXEL6("exs_pool_index_to_state(pool=%p, id=%u)", pool, id);
    SXEA1(array != NULL, "pool array is not null");
    SXEA1(id < POOL_POOL_INFO->num_elems, "index (%u) is out of range (%u total states)", id, POOL_POOL_INFO->num_elems);

    unsigned state = POOL_ELEMS_INFO[id].state;
    SXEL6("return %u", state);
    return state;
}

unsigned
exs_pool_get_oldest_element_index(void * array, unsigned state)
{
    char * pool = array;
    SXEL6("exs_pool_get_oldest_element_index(pool=%p, state=%u)", pool, state);
    SXEA1(array != NULL, "pool array is not null");
    SXEA1(state < POOL_POOL_INFO->num_states, "state (%u) is out of range (%u total states)", state, POOL_POOL_INFO->num_states);

    unsigned id = POOL_STATES_INFO[state].tail;
    SXEL6("return id=%u", id);
    return id;
}

unsigned
exs_pool_get_newest_element_index(void * array, unsigned state)
{
    char * pool = array;
    SXEL6("exs_pool_get_newest_element_index(pool=%p, state=%u)", pool, state);
    SXEA1(array != NULL, "pool array is not null");
    SXEA1(state < POOL_POOL_INFO->num_states, "state (%u) is out of range (%u total states)", state, POOL_POOL_INFO->num_states);

    unsigned id = POOL_STATES_INFO[state].head;
    SXEL6("return id=%u", id);
    return id;
}

unsigned
exs_pool_set_oldest_element_state(void * array, unsigned old_state, unsigned new_state)
{
    char * pool = array;
    SXEL6("exs_pool_set_oldest_element_state(pool=%p, name=%s, old_state=%u, new_state=%u)", pool, POOL_POOL_INFO->name, old_state, new_state);
    SXEA1(array != NULL, "pool array is not null");
    SXEA1(old_state < POOL_POOL_INFO->num_states, "old_state (%u) is out of range (%u total states)", old_state, POOL_POOL_INFO->num_states);
    SXEA1(new_state < POOL_POOL_INFO->num_states, "new_state (%u) is out of range (%u total states)", new_state, POOL_POOL_INFO->num_states);

    unsigned id = POOL_STATES_INFO[old_state].tail;
    if (id == POOL_NO_INDEX) { goto EARLY_OUT; }
    SXEA6(POOL_ELEMS_INFO[id].state == old_state, "The tail (%u) was in the correct old state (%u)", id, POOL_ELEMS_INFO[id].state);
    SXEA6(POOL_ELEMS_INFO[id].next == POOL_NO_INDEX, "The next (%u) of a tail (id=%u) is POOL_NO_INDEX", POOL_ELEMS_INFO[id].next, id);
    POOL_STATES_INFO[old_state].tail = POOL_ELEMS_INFO[id].prev;
    if (POOL_STATES_INFO[old_state].tail == POOL_NO_INDEX) {
        POOL_STATES_INFO[old_state].head = POOL_NO_INDEX;
    } else {
        POOL_ELEMS_INFO[POOL_ELEMS_INFO[id].prev].next = POOL_NO_INDEX;
    }
    POOL_STATES_INFO[old_state].count--;

    POOL_ELEMS_INFO[id].prev = POOL_NO_INDEX;
    POOL_ELEMS_INFO[id].next = POOL_STATES_INFO[new_state].head;
    POOL_STATES_INFO[new_state].head = id;
    if (POOL_ELEMS_INFO[id].next == POOL_NO_INDEX) {
        POOL_STATES_INFO[new_state].tail = id;
    } else {
        POOL_ELEMS_INFO[POOL_ELEMS_INFO[id].next].prev = id;
    }
    POOL_ELEMS_INFO[id].state = new_state;
    POOL_STATES_INFO[new_state].count++;

    gettimeofday(&(POOL_ELEMS_INFO[id].state_time), NULL);

EARLY_OUT:
    SXEL6("return id=%u", id);
    return id;
}

unsigned
exs_pool_set_newest_element_state(void * array, unsigned old_state, unsigned new_state)
{
    char * pool = array;
    SXEL6("exs_pool_set_newest_element_state(pool=%p, name=%s, old_state=%u, new_state=%u)", pool, POOL_POOL_INFO->name, old_state, new_state);
    SXEA1(array != NULL, "pool array is not null");
    SXEA1(old_state < POOL_POOL_INFO->num_states, "old_state (%u) is out of range (%u total states)", old_state, POOL_POOL_INFO->num_states);
    SXEA1(new_state < POOL_POOL_INFO->num_states, "new_state (%u) is out of range (%u total states)", new_state, POOL_POOL_INFO->num_states);

    unsigned id = POOL_STATES_INFO[old_state].head;
    if (id == POOL_NO_INDEX) { goto EARLY_OUT; }
    SXEA6(POOL_ELEMS_INFO[id].state == old_state, "The head (%u) was in the correct old state (%u)", id, POOL_ELEMS_INFO[id].state);
    SXEA6(POOL_ELEMS_INFO[id].prev == POOL_NO_INDEX, "The prev (%u) of a head (id=%u) is POOL_NO_INDEX", POOL_ELEMS_INFO[id].prev, id);
    POOL_STATES_INFO[old_state].head = POOL_ELEMS_INFO[id].next;
    if (POOL_STATES_INFO[old_state].head == POOL_NO_INDEX) {
        POOL_STATES_INFO[old_state].tail = POOL_NO_INDEX;
    } else {
        POOL_ELEMS_INFO[POOL_ELEMS_INFO[id].next].prev = POOL_NO_INDEX;
    }
    POOL_STATES_INFO[old_state].count--;

    POOL_ELEMS_INFO[id].next = POOL_NO_INDEX;
    POOL_ELEMS_INFO[id].prev = POOL_STATES_INFO[new_state].tail;
    POOL_STATES_INFO[new_state].tail = id;
    if (POOL_ELEMS_INFO[id].prev == POOL_NO_INDEX) {
        POOL_STATES_INFO[new_state].head = id;
    } else {
        POOL_ELEMS_INFO[POOL_ELEMS_INFO[id].prev].next = id;
    }
    POOL_ELEMS_INFO[id].state = new_state;
    POOL_STATES_INFO[new_state].count++;

    gettimeofday(&(POOL_ELEMS_INFO[id].state_time), NULL);

EARLY_OUT:
    SXEL6("return id=%u", id);
    return id;
}

unsigned
exs_pool_set_indexed_element_state(void * array, unsigned id, unsigned old_state, unsigned new_state)
{
    char * pool = array;
    SXEL6("exs_pool_set_indexed_element_state(pool=%p, id=%u, old_state=%u, new_state=%u)", pool, id, old_state, new_state);
    SXEA1(array != NULL, "pool array is not null");
    SXEA1(id < POOL_POOL_INFO->num_elems, "index is within range");
    SXEA1(old_state < POOL_POOL_INFO->num_states, "old_state (%u) is out of range (%u total states)", old_state, POOL_POOL_INFO->num_states);
    SXEA1(new_state < POOL_POOL_INFO->num_states, "new_state (%u) is out of range (%u total states)", new_state, POOL_POOL_INFO->num_states);
    SXEA1(new_state != old_state, "new_state (%u) != old_state (%u)", new_state, old_state);

    SXEA1(POOL_ELEMS_INFO[id].state == old_state, "Invalid state transition (pool=%s, id=%u) old_state=%u => new_state=%u, element is in state %u",
                                                  POOL_POOL_INFO->name, id, old_state, new_state, POOL_ELEMS_INFO[id].state);

    if (POOL_ELEMS_INFO[id].prev == POOL_NO_INDEX) {
        SXEA6(POOL_STATES_INFO[POOL_ELEMS_INFO[id].state].head == id, "id=%u is the head", id);
        POOL_STATES_INFO[POOL_ELEMS_INFO[id].state].head = POOL_ELEMS_INFO[id].next;
    } else {
        POOL_ELEMS_INFO[POOL_ELEMS_INFO[id].prev].next = POOL_ELEMS_INFO[id].next;
    }

    if (POOL_ELEMS_INFO[id].next == POOL_NO_INDEX) {
        SXEA6(POOL_STATES_INFO[POOL_ELEMS_INFO[id].state].tail == id, "id=%u is the tail", id);
        POOL_STATES_INFO[POOL_ELEMS_INFO[id].state].tail = POOL_ELEMS_INFO[id].prev;
    } else {
        POOL_ELEMS_INFO[POOL_ELEMS_INFO[id].next].prev = POOL_ELEMS_INFO[id].prev;
    }

    POOL_STATES_INFO[old_state].count--;
    POOL_ELEMS_INFO[id].state = new_state;
    POOL_STATES_INFO[new_state].count++;

    POOL_ELEMS_INFO[id].prev = POOL_NO_INDEX;
    if (POOL_STATES_INFO[new_state].head == POOL_NO_INDEX) {
        POOL_STATES_INFO[new_state].head = id;
        POOL_STATES_INFO[new_state].tail = id;
        POOL_ELEMS_INFO[id].next = POOL_NO_INDEX;
    } else {
        POOL_ELEMS_INFO[id].next = POOL_STATES_INFO[new_state].head;
        POOL_ELEMS_INFO[POOL_STATES_INFO[new_state].head].prev = id;
        POOL_STATES_INFO[new_state].head = id;
    }

    gettimeofday(&(POOL_ELEMS_INFO[id].state_time), NULL);

    SXEL6("return id=%u", id);
    return id;
}

unsigned
exs_pool_touch_indexed_element(void * array, unsigned id)
{
    char * pool = array;
    SXEL6("exs_pool_touch_indexed_element(pool=%p, id=%u)", pool, id);
    SXEA1(array != NULL, "pool array is not null");
    SXEA1(id < POOL_POOL_INFO->num_elems, "index is within range");

    unsigned state = POOL_ELEMS_INFO[id].state;

    if (POOL_STATES_INFO[state].count == 1) {
        // NOP
    } else if (POOL_STATES_INFO[state].head == id) {
        // NOP
    } else {
       POOL_ELEMS_INFO[POOL_ELEMS_INFO[id].prev].next = POOL_ELEMS_INFO[id].next;

       if (POOL_ELEMS_INFO[id].next == POOL_NO_INDEX) {
           POOL_STATES_INFO[state].tail = POOL_ELEMS_INFO[id].prev;
       } else {
           POOL_ELEMS_INFO[POOL_ELEMS_INFO[id].next].prev = POOL_ELEMS_INFO[id].prev;
       }

       POOL_ELEMS_INFO[id].prev = POOL_NO_INDEX;
       POOL_ELEMS_INFO[id].next = POOL_STATES_INFO[state].head;
       POOL_ELEMS_INFO[POOL_STATES_INFO[state].head].prev = id;
       POOL_STATES_INFO[state].head = id;
    }

    gettimeofday(&(POOL_ELEMS_INFO[id].state_time), NULL);

    return id;
}

struct timeval
exs_pool_get_oldest_element_time(void * array, unsigned state)
{
    char * pool = array;
    SXEL6("exs_pool_get_oldest_element_time(pool=%p, state=%u)", pool, state);
    SXEA1(array != NULL, "pool array is not null");
    SXEA1(state < POOL_POOL_INFO->num_states, "state (%u) is out of range (%u total states)", state, POOL_POOL_INFO->num_states);

    struct timeval diff;
    unsigned id = POOL_STATES_INFO[state].tail;

    if (id == POOL_NO_INDEX) {
        memset(&diff, '\0', sizeof(diff));
        return diff; 
    }

    struct timeval now;
    gettimeofday(&now, NULL);
    timersub(&now, &(POOL_ELEMS_INFO[id].state_time), &diff);
    return diff;
}

struct timeval
exs_pool_get_element_time_by_index(void * array, unsigned id)
{
    char * pool = array;
    SXEL6("exs_pool_get_element_time_by_index(pool=%p, id=%u)", pool, id);
    SXEA1(array != NULL, "pool array is not null");
    SXEA1(id < POOL_POOL_INFO->num_elems, "index is within range");

    struct timeval now;
    gettimeofday(&now, NULL);
    struct timeval diff;
    timersub(&now, &(POOL_ELEMS_INFO[id].state_time), &diff);
    return diff;
}

unsigned
exs_pool_index_if_older(void * array, unsigned state, unsigned seconds)
{
    char * pool = array;
    SXEL6("exs_pool_callback_if_older(pool=%p, state=%u)", pool, state);
    SXEA1(array != NULL, "pool array is not null");
    SXEA1(state < POOL_POOL_INFO->num_states, "state (%u) is out of range (%u total states)", state, POOL_POOL_INFO->num_states);

    unsigned id = POOL_STATES_INFO[state].tail;
    if (id == POOL_NO_INDEX) { return id; }

    struct timeval now;
    gettimeofday(&now, NULL);
    struct timeval diff;
    timersub(&now, &(POOL_ELEMS_INFO[id].state_time), &diff);

    if (diff.tv_sec >= seconds) { return id; }

    SXEL6("return id=%u", id);
    return POOL_NO_INDEX;
}

