#ifndef COMMON_BASE_CIRCULAR_QUEUE_H
#define COMMON_BASE_CIRCULAR_QUEUE_H

#include <array>
#include <cassert>
#include <memory>
#include <mutex>

namespace tinycommon {
namespace base {

template<typename T, size_t BufSize = 1 >
class circular_queue {
public:
    circular_queue();
    ~circular_queue();
    circular_queue(const circular_queue& from);

public:
    using value_type        = T;
    using size_type         = size_t;
    using index_type        = size_t;
    using reference         = T&;
    using const_reference   = const T&;
    using buffer_type       = std::array<T, BufSize>;
    using buffer_ptr_type   = std::shared_ptr<buffer_type>;

private:
    mutable std::mutex  m_mutex;

    buffer_ptr_type     m_buffer;

    size_type           m_capacity;     // 队列容量
    index_type          m_head;         // oldest
    index_type          m_tail;         // newest
    bool                m_isEmpty;      // 标志缓冲区是否为空

public:

    /// Empty
    /// \brief Empty 环状队列是否为空
    /// \return bool [true]：队列为空 [false]：队列不为空
    ///
    bool empty() const {
        return m_isEmpty;
    }

    /// Front
    /// \brief Front 返回队列头对象的拷贝
    /// \return T 队列头对象的拷贝
    /// @warning 为保证线程安全，返回值为对象的拷贝
    value_type front() const {
        index_type head, tail;
        bool isEmpty;
        buffer_ptr_type buffer = _get_buffer(head, tail, isEmpty);

        assert(isEmpty == false);
        return _at(head, buffer);
    }

    /// Back
    /// \brief Back 返回队列尾对象的拷贝
    /// \return T 队列为对象的拷贝
    /// @warning 为保证线程安全，返回值为对象的拷贝
    value_type back() const {
        index_type head, tail;
        bool isEmpty;
        buffer_ptr_type buffer = _get_buffer(head, tail, isEmpty);

        assert(isEmpty == false);
        return _at(m_tail, buffer);
    }


    /// Size
    /// \brief Size 返回当前队列中的对象数量
    /// \return size_t 当前队列中的对象数量
    ///
    size_type size() const {
        index_type head, tail;
        bool isEmpty;
        buffer_ptr_type buffer = _get_buffer(head, tail, isEmpty);
        return _get_size(head, tail, isEmpty);
    }

    /// Capacity
    /// \brief Capacity 返回队列容量
    /// \return
    ///
    size_type capacity() const {
        return m_capacity;
    }

    ///
    /// \brief operator []
    /// \param n
    /// \return 队列中第n个对象的拷贝
    /// @details 索引计算方法：Front对象（oldest）索引为0，Back对象（newest）索引为Size()
    /// @warning 为保证线程安全，返回值为对象的拷贝
    ///
    value_type operator[](index_type n) const {
        index_type head, tail;
        bool isEmpty;
        buffer_ptr_type buffer = _get_buffer(head, tail, isEmpty);

        assert(n < _get_size(head, tail, isEmpty));

        return _at(_index_add(head, n), buffer);
    }

    circular_queue<T,BufSize>& operator=(const circular_queue& from) {
        index_type head, tail;
        bool isEmpty;
        buffer_ptr_type buffer = from._get_buffer(head, tail, isEmpty);

        std::lock_guard<std::mutex> lck (m_mutex);

        m_buffer.reset(new buffer_type(*buffer));
        m_head = head;
        m_tail = tail;
        m_isEmpty = isEmpty;

        m_capacity = from.m_capacity;

        return *this;
    }

    /// PushBack
    /// \brief PushBack 向队列尾部追加对象
    /// \param from
    /// @warning 在队列Size() == Capacity() 时，此操作将会使用from覆盖掉Front对象（oldest）
    /// @warning 传入类型需已重载赋值运算符
    ///
    void push_back(const reference from);

    ///
    /// \brief Pop 弹出队列当前头部对象
    /// \return 队列头部对象的拷贝
    ///
    value_type pop();

private:

    index_type _index_add(index_type index, size_type n) const {
        return (index + n) % m_capacity;
    }

    reference _at(index_type n, buffer_ptr_type& buffer) const {
        return (*buffer)[n];
    }

    /// \brief 提供buffer（智能指针）的拷贝，线程安全需要
    buffer_ptr_type _get_buffer(index_type& head, index_type& tail, bool& isEmpty) const {
        std::lock_guard<std::mutex> lck (m_mutex);
        head = m_head;
        tail = m_tail;
        isEmpty = m_isEmpty;
        return m_buffer;
    }

    size_type _get_size(const index_type& head, const index_type& tail, const bool& isEmpty) const {
        if (isEmpty == true) {
            return 0;
        } else if (tail >= head) {
            return tail - head + 1;
        } else {
            return m_capacity - (head - tail - 1);
        }
    }
};


template<typename T, size_t BufSize>
circular_queue<T, BufSize>::circular_queue():m_buffer(new buffer_type),
                                           m_capacity(BufSize),
                                           m_head(0),m_tail(0),
                                           m_isEmpty(true) {
}

template<typename T, size_t BufSize>
circular_queue<T, BufSize>::circular_queue(const circular_queue &from) {
    index_type head, tail;
    bool isEmpty;
    buffer_ptr_type buffer = from._get_buffer(head, tail, isEmpty);

    std::lock_guard<std::mutex> lck (m_mutex);

    m_buffer.reset(new buffer_type(*buffer));
    m_head = head;
    m_tail = tail;
    m_isEmpty = isEmpty;

    m_capacity = from.m_capacity;
}

template<typename T, size_t BufSize>
circular_queue<T, BufSize>::~circular_queue() {
}

template<typename T, size_t BufSize>
void circular_queue<T, BufSize>::push_back(const reference from) {
    // copy on write, 确保读数据的一致性
    std::lock_guard<std::mutex> lck (m_mutex);
    if (!m_buffer.unique()) {
        m_buffer.reset(new buffer_type(*m_buffer));
    }

    // 队列已满，head向后移动
    if (_index_add(m_tail, 1) == m_head) {
        m_head = _index_add(m_head, 1);
    }
    //队列为空时，不需要移动tail
    if (m_isEmpty == false) {
        m_tail = _index_add(m_tail, 1);
    }

    (*m_buffer)[m_tail] = from;

    m_isEmpty = false;
}

template<typename T, size_t BufSize>
typename circular_queue<T,BufSize>::value_type circular_queue<T, BufSize>::pop() {
    // copy on write, 确保读数据的一致性
    std::lock_guard<std::mutex> lck (m_mutex);
    if (!m_buffer.unique()) {
        m_buffer.reset(new buffer_type(*m_buffer));
    }

    assert(m_isEmpty == false);

    index_type preHead = m_head;
    m_head = _index_add(m_head, 1);

    // 若取出元素后，队列为空，重置tail与head
    if (m_head == _index_add(m_tail,1)) {
        m_tail = m_head;
        m_isEmpty = true;
    }

    return _at(preHead, m_buffer);
}


} // namespace Base
} // namespace TinyCommon

#endif
