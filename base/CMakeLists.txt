
add_executable(circular_queue_utest utest/circular_queue_utest.cpp)
add_executable(lru_cache_utest utest/lru_cache_utest.cpp)

target_link_libraries(lru_cache_utest gtest pthread)
target_link_libraries(circular_queue_utest gtest pthread)
