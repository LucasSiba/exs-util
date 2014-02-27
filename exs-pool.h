#ifndef __EXS_POOL_H__
#define __EXS_POOL_H__

#define POOL_NO_INDEX -1U

void * exs_pool_new(const char * name, unsigned num_elemsr, size_t sizeof_elem, unsigned num_states);
void   exs_pool_del(void * array);

unsigned exs_pool_get_number_in_state(void * array, unsigned state);
unsigned exs_pool_index_to_state(void * array, unsigned id);

unsigned exs_pool_get_oldest_element_index(void * array, unsigned state);
unsigned exs_pool_get_newest_element_index(void * array, unsigned state);

unsigned exs_pool_set_oldest_element_state(void * array, unsigned old_state, unsigned new_state);
unsigned exs_pool_set_newest_element_state(void * array, unsigned old_state, unsigned new_state);

unsigned exs_pool_set_indexed_element_state(void * array, unsigned id, unsigned old_state, unsigned new_state);
unsigned exs_pool_touch_indexed_element(void * array, unsigned id);

struct timeval exs_pool_get_oldest_element_time(void * array, unsigned state);
struct timeval exs_pool_get_element_time_by_index(void * array, unsigned id);

unsigned exs_pool_index_if_older(void * array, unsigned state, unsigned seconds);

// Test only
void test_verify_pool_internals(void * array);

#endif /* __EXS_POOL_H__ */
