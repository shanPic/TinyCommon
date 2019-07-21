#include <assert.h>
#include <gtest/gtest.h>
#include <iostream>
#include <stdio.h>
#include <sys/time.h>

#include "../circular_queue.h"

namespace tinycommon {
namespace base {

uint64_t get_microsecond() {
    timespec time_now;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &time_now);
    return static_cast<uint64_t>(time_now.tv_nsec);
}

TEST(CircularQueueTest, CQConstruct) {
    circular_queue<int> cq0;
    ASSERT_EQ(0U, cq0.size());
    ASSERT_EQ(1U, cq0.capacity()); // 默认BufferSize为1
    ASSERT_EQ(true, cq0.empty());

    unsigned int n = 3;
    circular_queue<int, 3> cq1;
    ASSERT_EQ(0U, cq1.size());
    ASSERT_EQ(n, cq1.capacity());

    // 复制构造函数
    circular_queue<int, 3> cq2(cq1);
    ASSERT_EQ(0U, cq2.size());
    ASSERT_EQ(n, cq2.capacity());

    const circular_queue<int, 3> cq3(cq1);
    ASSERT_EQ(0U, cq3.size());
    ASSERT_EQ(n, cq3.capacity());

    // 赋值运算符
    circular_queue<int, 3> cq4;
    cq4 = cq1;
    ASSERT_EQ(0U, cq4.size());
    ASSERT_EQ(n, cq4.capacity());
}

TEST(CircularQueueTest, CQPush) {
    int n = 10;
    circular_queue<int, 10> cq;

    // 非覆盖写入
    for (int i = 1; i <= n; ++i) {
        cq.push_back(i);
        ASSERT_EQ((unsigned int)i, cq.size());
        ASSERT_EQ(0, cq.empty());
        EXPECT_EQ(1, cq.front());
        EXPECT_EQ(i, cq.back());

        // 测试重载的下标运算符
        for (int j = 0; j < i; ++j) {
            EXPECT_EQ(j + 1, cq[(unsigned int)j]);
        }
        for (int j = i; j <= 2*n; ++j) {
            EXPECT_DEATH(cq[(unsigned int)j], "");
        }
    }

    EXPECT_EQ(1, cq.front());
    EXPECT_EQ(n, cq.back());

    // 覆盖写入
    for (int i = n+1; i <= 2*n; ++i) {
        cq.push_back(i);
        ASSERT_EQ((unsigned int)n, cq.size());
        EXPECT_EQ(i - n + 1, cq.front());
        EXPECT_EQ(i, cq.back());

        // 测试重载的下标运算符
        for (int j = 0; j < n; ++j) {
            EXPECT_EQ(i - n + 1 + j, cq[(unsigned int)j]);
        }
        for (int j = i; j <= 2*n; ++j) {
            EXPECT_DEATH(cq[(unsigned int)j], "");
        }
    }

}

TEST(CircularQueueTest, CQPop) {

    int n = 6;
    circular_queue<int, 6> cq;

    // 非覆盖写入
    for (int i = 1; i <= n; ++i) {
        cq.push_back(i);
        ASSERT_EQ(0U, cq.empty());
    }

    for (int i = 1; i <= n; ++i) {
        EXPECT_EQ(i, cq.pop());

        // 测试重载的下标运算符
        for (int j = 0; j < n-i; ++j) {
            EXPECT_EQ(j + i + 1, cq[j]);
        }
        for (int j = n-i; j <= n; ++j) {
            EXPECT_DEATH(cq[(unsigned int)j], "");
        }
    }

    ASSERT_EQ(true, cq.empty());
    EXPECT_DEATH(cq.pop(), "");

    for (int i = 1; i <= n; ++i) {
        cq.push_back(i);
        EXPECT_EQ(i, cq.pop());
    }

    ASSERT_EQ(true, cq.empty());
    EXPECT_DEATH(cq.pop(), "");

    //覆盖写入
    for (int i = 1; i <= 2*n; ++i) {
        cq.push_back(i);
    }

    for (int i = 1; i <= n; ++i) {
        EXPECT_EQ(i + n, cq.pop());

        // 测试重载的下标运算符
        for (int j = 0; j < n-i; ++j) {
            EXPECT_EQ(j + i + n + 1, cq[(unsigned int)j]);
        }
        for (int j = n-i; j <= n; ++j) {
            EXPECT_DEATH(cq[(unsigned int)j], "");
        }
    }

    ASSERT_EQ(true, cq.empty());
    EXPECT_DEATH(cq.pop(), "");

    for (int i = 1; i <= 2*n; ++i) {
        cq.push_back(i);
        EXPECT_EQ(i, cq.pop());
    }

    ASSERT_EQ(true, cq.empty());
    EXPECT_DEATH(cq.pop(), "");
}

TEST(CircularQueueTest, CQPerformance) {
    const int cap = 300000;

    circular_queue<int, cap> cq0;

    uint64_t totalTime = 0;
    for (int i = 0; i < cap; ++i) {
        uint64_t begin = get_microsecond();
        cq0.push_back(i);
        uint64_t end = get_microsecond();

        totalTime += end - begin;
    }

    std::cout << "average latency with no-rewrite push:"
                << 1.0 * totalTime / cap << std::endl;

    totalTime = 0;
    for (int i = 0; i < cap; ++i) {
        uint64_t begin = get_microsecond();
        cq0.push_back(i);
        uint64_t end = get_microsecond();

        totalTime += end - begin;
    }

    std::cout << "average latency with rewrite push:"
                << 1.0 * totalTime / cap << std::endl;

    totalTime = 0;
    for (int i = 0; i < cap; ++i) {
        uint64_t begin = get_microsecond();
        cq0.pop();
        uint64_t end = get_microsecond();

        totalTime += end - begin;
    }

    std::cout << "average latency with pop:"
                << 1.0 * totalTime / cap << std::endl;

}

}// namespace common
}// namespace mapauto

int main(int argc,char *argv[])
{
    testing::InitGoogleTest(&argc, argv);//将命令行参数传递给gtest
    return RUN_ALL_TESTS();   //RUN_ALL_TESTS()运行所有测试案例
}
