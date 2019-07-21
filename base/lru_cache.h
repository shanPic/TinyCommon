#ifndef COMMON_BASE_LRU_CACHE_H
#define COMMON_BASE_LRU_CACHE_H

#include <list>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace tinycommon{
namespace base {

/// LRU cache base on elements count and memory size
template <typename Key, typename Value>
class LRU_cache
{
public:
    using key_type          = Key;
    using value_type        = Value;
    using value_ptr_type    = std::shared_ptr<Value>;
    using list_value        = std::pair<Key, value_ptr_type>;
    using iterator          = typename std::list<list_value>::iterator;
    using size_type         = size_t;
    using rate_type         = double;

    struct Stats {
        size_type   m_get_cnt; //总的请求次数
        size_type   m_hit_cnt; //命中次数
    };

private:
    mutable std::mutex              m_mutex;
    std::list<list_value>           m_list;
    std::unordered_map<key_type, iterator>  m_hash_table;

    size_type                       m_max_size;         // 可保有的最大元素数量
    size_type                       m_max_memory_size;  // 最大内存大小

    Stats                           m_stats;

public:
    LRU_cache() = delete;

    ///
    /// construct
    /// \param [Size] 最大元素个数
    ///
    LRU_cache(size_type Size);

    ///
    /// construct
    /// \param [Size] 最大元素个数，[MemorySize] 最大内存大小（以字节为单位）
    ///
    LRU_cache(size_type Size, size_type MemorySize);

    LRU_cache(const LRU_cache& from);

public:
    ///
    /// push
    /// \brief 将k-v对压入容器
    /// \param [in]: key, value
    /// \warning 压入后放在队头，若压入后满足了进行淘汰的条件，将淘汰1个元素
    ///
    void push(const key_type& key, const value_ptr_type& value);

    ///
    /// get
    /// \brief 根据key，从容器中取出value
    /// \param [in]: key, [out]: value
    /// \return bool [ture]: 容器中有此k-v对 [false]: 容器中无此k-v对
    /// \warning 若容器中有此k-v对，get操作会根据最近使用原则，将此k-v对移动至队头
    ///
    bool get(const key_type& key, value_ptr_type& value);

    ///
    /// exists
    /// \brief 判断容器中是否有key对应的k-v对
    /// \param [in]: key
    /// \return bool [ture]: 容器中有此k-v对 [false]: 容器中无此k-v对
    /// \warning  此方法不会影响元素位置，不更该容器状态，只返回k-v对是否存在
    ///
    bool exists(const key_type& key) const
    {
        std::lock_guard<std::mutex> lck(m_mutex);
        auto ite = m_hash_table.find(key);
        return ite == m_hash_table.end() ? false : true;
    }

    ///
    /// get_hit_rate
    /// \brief 获取截至当前get的命中率
    /// \return rate_type(double)
    ///
    rate_type get_hit_rate() const
    {
        std::lock_guard<std::mutex> lck(m_mutex);
        return static_cast<rate_type>(m_stats.m_hit_cnt) /
            static_cast<rate_type>(m_stats.m_get_cnt);
    }

    ///
    /// reset_stats
    /// \brief 重置容器状态，重新统计命中率
    ///
    void reset_stats()
    {
        std::lock_guard<std::mutex> lck (m_mutex);
        m_stats.m_hit_cnt = 0;
        m_stats.m_get_cnt = 0;
    }

public:
    LRU_cache<Key, Value>& operator=(const LRU_cache& from)
    {
        BD_SLOCK(this->m_mutex);
        this->m_list = from.m_list;
        this->m_hash_table = from.m_hash_table;

        this->m_max_size = from.m_max_size;
        this->m_max_memory_size = from.m_max_memory_size;

        this->m_get_cnt = from.m_stats.m_get_cnt;
        this->m_hit_cnt = from.m_stats.m_hit_cnt;
    }

private:
    ///
    /// [内部方法] 获取当前保有的元素个数
    /// \return size_type
    ///
    size_type _get_cache_size() const
    {
        // 此处使用hashmap的size，时间复杂度为O(1)。list求size的时间复杂度为O(n)
        return m_hash_table.size();
    }

    ///
    /// [内部方法] 获取当前占用内存大小
    /// \return size_type
    ///
    size_type _get_memory_size() const
    {
        return _get_cache_size() * sizeof(value_type);
    }

    ///
    /// [内部方法] 淘汰一个元素，时间复杂度O(1)
    ///
    void _discard_one_elem()
    {
        // std::list为双向链表，end()的时间复杂度为O(1)
        auto ite_list_last = m_list.end();
        ite_list_last--;
        m_hash_table.erase(ite_list_last->first);
        m_list.erase(ite_list_last);
    }
};

template <typename Key, typename Value>
LRU_cache<Key, Value>::LRU_cache(size_t Size) :
    m_max_size(Size),
    m_max_memory_size(0)
{
    m_stats.m_get_cnt = 0;
    m_stats.m_hit_cnt = 0;
}

template <typename Key, typename Value>
LRU_cache<Key, Value>::LRU_cache(size_type Size, size_type MemorySize) :
    m_max_size(Size),
    m_max_memory_size(MemorySize)
{
    m_stats.m_get_cnt = 0;
    m_stats.m_hit_cnt = 0;
}

template <typename Key, typename Value>
LRU_cache<Key, Value>::LRU_cache(const LRU_cache& from)
{
    std::lock_guard<std::mutex> lck (m_mutex);
    this->m_list = from.m_list;
    this->m_hash_table = from.m_hash_table;

    this->m_max_size = from.m_max_size;
    this->m_max_memory_size = from.m_max_memory_size;

    this->m_get_cnt = from.m_stats.m_get_cnt;
    this->m_hit_cnt = from.m_stats.m_hit_cnt;
}

template <typename Key, typename Value>
void LRU_cache<Key, Value>::push(const key_type& key, const value_ptr_type& value) {
    std::lock_guard<std::mutex> lck (m_mutex);

    auto ite = m_hash_table.find(key);
    if (ite == m_hash_table.end()) {
        m_list.push_front({key, value});
        m_hash_table[key] = m_list.begin();
    } else {
        ite->second->second = value;
        m_list.splice(m_list.begin(), m_list, ite->second);
    }

    // 需同时满足size与内存大小两个条件才不会进行淘汰
    if (_get_cache_size() > m_max_size ||
        (m_max_memory_size != 0 && _get_memory_size() > m_max_memory_size)) {
        _discard_one_elem();
    }

}

template <typename Key, typename Value>
bool LRU_cache<Key, Value>::get(const key_type& key, value_ptr_type& value)
{
    std::lock_guard<std::mutex> lck (m_mutex);

    m_stats.m_get_cnt++;
    auto ite = m_hash_table.find(key);

    if (ite == m_hash_table.end()) {
        return false;
    }

    m_stats.m_hit_cnt++;
    m_list.splice(m_list.begin(), m_list, ite->second);

    value = ite->second->second;
    return true;
}

} // namespace tinycommo
} // namespace base
#endif
