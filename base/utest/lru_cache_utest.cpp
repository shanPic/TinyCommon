#include <assert.h>
#include <gtest/gtest.h>

#include <memory>
#include <iostream>
#include <string>

#include "../lru_cache.h"
#include "posix_time.h"

namespace tinycommon {
namespace base {

TEST(LRUCacheTest, LRUCacheConstruct) {

}

TEST(LRUCacheTest, LRUCachePushAndGet) {
    int n = 30;
    LRU_cache<int, int> lru0(30);

    for (int i = 0; i < n; ++i) {
        lru0.push(i, std::make_shared<int>(i + n));
    }

    for (int i = 0; i < n; ++i) {
        ASSERT_EQ(1, lru0.exists(i));
        auto tmp = std::make_shared<int>(-1);
        ASSERT_EQ(1, lru0.get(i, tmp));
        ASSERT_EQ(i + n, *tmp);
    }

    // check twice
    for (int i = 0; i < n; ++i) {
        ASSERT_EQ(1, lru0.exists(i));
        auto tmp = std::make_shared<int>(-1);
        ASSERT_EQ(1, lru0.get(i, tmp));
        ASSERT_EQ(i + n, *tmp);
    }

    ASSERT_EQ(0, lru0.exists(n));
    auto ret = std::make_shared<int>(-1);;
    ASSERT_EQ(0, lru0.get(n, ret));
}

TEST(LRUCacheTest, LRUCacheDiscard) {
    // 根据保有元素个数进行淘汰
    int n = 3;
    LRU_cache<int, int> lru1(static_cast<size_t >(n));

    for (int i = 0; i < n; ++i) {
        lru1.push(i, std::make_shared<int>(i + n));
    }

    for (int i = 0; i < n; ++i) {
        auto tmp = std::make_shared<int>(-1);
        EXPECT_EQ(1, lru1.get(i, tmp));
        EXPECT_EQ(i + n, *tmp);
    }

    for (int i = n; i < n * 2; ++i) {
        lru1.push(i, std::make_shared<int>(i + n));
        ASSERT_EQ(1, lru1.exists(i));
        int j = 0;
        for (; j <= i - n; ++j) {
            EXPECT_EQ(0, lru1.exists(j));
        }
        for (; j <= i; ++j) {
            EXPECT_EQ(1, lru1.exists(j));
        }
    }

    // 根据内存大小进行淘汰
    n = 3;
    LRU_cache<int, int> lru2(static_cast<size_t >(n * 2), n * sizeof(int));
    for (int i = 0; i < n; ++i) {
        lru2.push(i, std::make_shared<int>(i + n));
    }

    for (int i = 0; i < n; ++i) {
        auto tmp = std::make_shared<int>(-1);
        EXPECT_EQ(1, lru2.get(i, tmp));
        EXPECT_EQ(i + n, *tmp);
    }

    for (int i = n; i < n * 2; ++i) {
        lru2.push(i, std::make_shared<int>(i + n));
        ASSERT_EQ(1, lru2.exists(i));
        int j = 0;
        for (; j <= i - n; ++j) {
            EXPECT_EQ(0, lru2.exists(j));
        }
        for (; j <= i; ++j) {
            EXPECT_EQ(1, lru2.exists(j));
        }
    }
}

TEST(LRUCacheTest, LRUCacheTestHitRate) {
    int n = 10000;
    LRU_cache<int, int> lru3(10000);

    for (int i = 0; i < n; ++i) {
        lru3.push(i, std::make_shared<int>(i + n));
    }

    // 无淘汰情况下的命中率
    auto tmp =  std::make_shared<int>(-1);
    for (int i = 0; i < n * 2; ++i) {
        lru3.get(i, tmp);
    }

    EXPECT_DOUBLE_EQ(0.5, lru3.get_hit_rate());

    // 有淘汰情况下的命中率
    for (int i = n; i < n * 2; ++i) {
        lru3.push(i, std::make_shared<int>(i + n));
    }

    //tmp = -1;
    for (int i = n; i < n * 4; ++i) {
        lru3.get(i, tmp);
    }

    EXPECT_DOUBLE_EQ(0.4, lru3.get_hit_rate());

}

TEST(LRUCacheTest, LRUCachePerformance) {
    const int cap = 3000000;

    LRU_cache<int, int> lru4(static_cast<size_t >(cap));

    uint64_t totalTime = 0;
    for (int i = 0; i < cap; ++i) {
        uint64_t begin = get_microsecond();
        lru4.push(i, std::make_shared<int>(i));
        uint64_t end = get_microsecond();

        totalTime += end - begin;
    }

    std::cout << "average latency with no-discard push:"
              << 1.0 * totalTime / cap << " us" << std::endl;

    totalTime = 0;
    for (int i = cap; i < cap * 2; ++i) {
        uint64_t begin = get_microsecond();
        lru4.push(i, std::make_shared<int>(i));
        uint64_t end = get_microsecond();

        totalTime += end - begin;
    }

    std::cout << "average latency with discard push:"
              << 1.0 * totalTime / cap << " us" << std::endl;

    totalTime = 0;
    for (int i = cap; i < cap * 2; ++i) {
        auto tmp = std::make_shared<int>(-1);
        uint64_t begin = get_microsecond();
        lru4.get(i, tmp);
        uint64_t end = get_microsecond();

        totalTime += end - begin;
    }

    std::cout << "average latency with get exists elements:"
              << 1.0 * totalTime / cap << " us" << std::endl;

    totalTime = 0;
    for (int i = 0; i < cap; ++i) {
        auto tmp = std::make_shared<int>(-1);
        uint64_t begin = get_microsecond();
        lru4.get(i, tmp);
        uint64_t end = get_microsecond();

        totalTime += end - begin;
    }

    std::cout << "average latency with get not exists elements:"
              << 1.0 * totalTime / cap << " us" << std::endl;
}

}// namespace common
}// namespace mapauto
