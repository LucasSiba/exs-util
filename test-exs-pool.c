#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

#include "tap.h"
#include "exs-pool.h"

typedef struct test_pool_element {
    unsigned num1;
    unsigned num2;
} test_pool_element;

typedef enum TEST_POOL_STATE {
    TEST_POOL_STATE_1=0,
    TEST_POOL_STATE_2,
    TEST_POOL_STATE_3,
    TEST_POOL_STATE_NUMBER_OF_STATES
} TEST_POOL_STATE;

int
main(void)
{
    plan_tests(78);

    test_pool_element* pool = (test_pool_element*) exs_pool_new("test-pool-name", 3, sizeof(test_pool_element), TEST_POOL_STATE_NUMBER_OF_STATES);

    pool[0].num1 = 1;
    pool[0].num2 = 2;
    pool[1].num1 = 3;
    pool[1].num2 = 4;
    pool[2].num1 = 5;
    pool[2].num2 = 6;

    is(exs_pool_get_number_in_state(pool, 0), 3, "ok");
    is(exs_pool_get_number_in_state(pool, 1), 0, "ok");
    is(exs_pool_get_number_in_state(pool, 2), 0, "ok");
    is(exs_pool_index_to_state(pool, 0), 0, "ok");
    is(exs_pool_index_to_state(pool, 1), 0, "ok");
    is(exs_pool_index_to_state(pool, 2), 0, "ok");
    is(exs_pool_get_oldest_element_index(pool, 0), 2, "ok");
    is(exs_pool_get_oldest_element_index(pool, 1), POOL_NO_INDEX, "ok");
    is(exs_pool_get_oldest_element_index(pool, 2), POOL_NO_INDEX, "ok");
    is(exs_pool_get_newest_element_index(pool, 0), 0, "ok");
    is(exs_pool_get_newest_element_index(pool, 1), POOL_NO_INDEX, "ok");
    is(exs_pool_get_newest_element_index(pool, 2), POOL_NO_INDEX, "ok");

    test_verify_pool_internals(pool);

    is(exs_pool_set_oldest_element_state(pool, 0, 1), 2, "ok");
    is(exs_pool_set_oldest_element_state(pool, 0, 1), 1, "ok");
    is(exs_pool_set_oldest_element_state(pool, 0, 1), 0, "ok");
    is(exs_pool_set_oldest_element_state(pool, 0, 1), POOL_NO_INDEX, "ok");

    test_verify_pool_internals(pool);

    is(exs_pool_get_number_in_state(pool, 0), 0, "ok");
    is(exs_pool_get_number_in_state(pool, 1), 3, "ok");
    is(exs_pool_get_number_in_state(pool, 2), 0, "ok");

    is(exs_pool_set_newest_element_state(pool, 1, 2), 0, "ok");
    is(exs_pool_set_newest_element_state(pool, 1, 2), 1, "ok");
    is(exs_pool_set_newest_element_state(pool, 1, 2), 2, "ok");
    is(exs_pool_set_newest_element_state(pool, 1, 2), POOL_NO_INDEX, "ok");

    test_verify_pool_internals(pool);

    is(exs_pool_set_indexed_element_state(pool, 0, 2, 0), 0, "ok");
    is(exs_pool_set_indexed_element_state(pool, 1, 2, 1), 1, "ok");
    is(exs_pool_get_number_in_state(pool, 0), 1, "ok");
    is(exs_pool_get_number_in_state(pool, 1), 1, "ok");
    is(exs_pool_get_number_in_state(pool, 2), 1, "ok");
    is(exs_pool_set_indexed_element_state(pool, 1, 1, 0), 1, "ok");
    is(exs_pool_set_indexed_element_state(pool, 2, 2, 0), 2, "ok");
    is(exs_pool_get_number_in_state(pool, 0), 3, "ok");
    is(exs_pool_get_number_in_state(pool, 1), 0, "ok");
    is(exs_pool_get_number_in_state(pool, 2), 0, "ok");
    is(exs_pool_set_indexed_element_state(pool, 0, 0, 2), 0, "ok");
    is(exs_pool_set_indexed_element_state(pool, 1, 0, 2), 1, "ok");
    is(exs_pool_set_indexed_element_state(pool, 2, 0, 2), 2, "ok");
    is(exs_pool_get_number_in_state(pool, 0), 0, "ok");
    is(exs_pool_get_number_in_state(pool, 1), 0, "ok");
    is(exs_pool_get_number_in_state(pool, 2), 3, "ok");

    test_verify_pool_internals(pool);

    is(exs_pool_set_indexed_element_state(pool, 0, 2, 0), 0, "ok");
    is(exs_pool_touch_indexed_element(pool, 0), 0, "ok");
    is(exs_pool_set_indexed_element_state(pool, 1, 2, 0), 1, "ok");
    is(exs_pool_get_number_in_state(pool, 0), 2, "ok");
    is(exs_pool_get_oldest_element_index(pool, 0), 0, "ok");
    is(exs_pool_get_newest_element_index(pool, 0), 1, "ok");

    test_verify_pool_internals(pool);

    is(exs_pool_set_indexed_element_state(pool, 2, 2, 0), 2, "ok");
    is(exs_pool_get_oldest_element_index(pool, 0), 0, "ok");
    is(exs_pool_get_newest_element_index(pool, 0), 2, "ok");
    is(exs_pool_touch_indexed_element(pool, 1), 1, "ok");
    is(exs_pool_get_oldest_element_index(pool, 0), 0, "ok");
    is(exs_pool_get_newest_element_index(pool, 0), 1, "ok");

    test_verify_pool_internals(pool);

    is(exs_pool_get_number_in_state(pool, 0), 3, "ok");
    is(exs_pool_get_number_in_state(pool, 1), 0, "ok");
    is(exs_pool_get_number_in_state(pool, 2), 0, "ok");

    is(exs_pool_touch_indexed_element(pool, 1), 1, "ok");
    is(exs_pool_get_oldest_element_index(pool, 0), 0, "ok");
    is(exs_pool_get_newest_element_index(pool, 0), 1, "ok");
    is(exs_pool_touch_indexed_element(pool, 2), 2, "ok");
    is(exs_pool_get_oldest_element_index(pool, 0), 0, "ok");
    is(exs_pool_get_newest_element_index(pool, 0), 2, "ok");
    is(exs_pool_touch_indexed_element(pool, 0), 0, "ok");
    is(exs_pool_get_oldest_element_index(pool, 0), 1, "ok");
    is(exs_pool_get_newest_element_index(pool, 0), 0, "ok");

    test_verify_pool_internals(pool);

    is(pool[0].num1, 1, "ok");
    is(pool[0].num2, 2, "ok");
    is(pool[1].num1, 3, "ok");
    is(pool[1].num2, 4, "ok");
    is(pool[2].num1, 5, "ok");
    is(pool[2].num2, 6, "ok");

    struct timeval id_zero = exs_pool_get_element_time_by_index(pool, 0);
    printf("id=0, state_time=%ld.%06ld\n", id_zero.tv_sec, id_zero.tv_usec);
    struct timeval id_one = exs_pool_get_element_time_by_index(pool, 1);
    printf("id=1, state_time=%ld.%06ld\n", id_one.tv_sec, id_one.tv_usec);
    struct timeval id_two = exs_pool_get_element_time_by_index(pool, 2);
    printf("id=2, state_time=%ld.%06ld\n", id_two.tv_sec, id_two.tv_usec);

    test_verify_pool_internals(pool);

    is(!!timercmp(&id_zero, &id_one, <=), 1, "ok");
    is(!!timercmp(&id_zero, &id_two, <=), 1, "ok");
    is(!!timercmp(&id_two, &id_one, >=), 0, "ok");
    is(!!timercmp(&id_two, &id_zero, >=), 1, "ok");

    struct timeval state_zero = exs_pool_get_oldest_element_time(pool, 0);
    printf("state=0, state_time=%ld.%06ld\n", state_zero.tv_sec, state_zero.tv_usec);
    struct timeval state_one = exs_pool_get_oldest_element_time(pool, 1);
    printf("state=1, state_time=%ld.%06ld\n", state_one.tv_sec, state_one.tv_usec);
    struct timeval state_two = exs_pool_get_oldest_element_time(pool, 2);
    printf("state=2, state_time=%ld.%06ld\n", state_two.tv_sec, state_two.tv_usec);

    is(!!timercmp(&state_one, &state_two, ==), 1, "ok");
    is(!!timercmp(&state_zero, &state_one, >), 1, "ok");

    sleep(1); // sucks! :(

    is(exs_pool_index_if_older(pool, 0, 1), 1, "ok");
    is(exs_pool_index_if_older(pool, 0, 2), POOL_NO_INDEX, "ok");
    is(exs_pool_index_if_older(pool, 1, 1), POOL_NO_INDEX, "ok");

    test_verify_pool_internals(pool);

    exs_pool_del(pool);

    return exit_status();
}
