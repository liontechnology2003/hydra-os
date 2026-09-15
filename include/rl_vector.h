#ifndef LIB_RL_VECTOR_H
#define LIB_RL_VECTOR_H

#include "libcpp.h"

namespace rl {

template<typename T>
class vector {
public:
    vector() : m_data(nullptr), m_size(0), m_capacity(0) {}

    explicit vector(uint32 count) : m_data(nullptr), m_size(0), m_capacity(0) {
        resize(count);
    }

    vector(uint32 count, const T &value) : m_data(nullptr), m_size(0), m_capacity(0) {
        resize(count);
        for (uint32 i = 0; i < count; i++) m_data[i] = value;
    }

    vector(const vector &other) : m_data(nullptr), m_size(0), m_capacity(0) {
        if (other.m_size > 0) {
            reserve(other.m_size);
            for (uint32 i = 0; i < other.m_size; i++) {
                m_data[i] = other.m_data[i];
            }
            m_size = other.m_size;
        }
    }

    vector(vector &&other) noexcept
        : m_data(other.m_data), m_size(other.m_size), m_capacity(other.m_capacity)
    {
        other.m_data = nullptr;
        other.m_size = 0;
        other.m_capacity = 0;
    }

    ~vector() {
        if (m_data) kfree(m_data);
    }

    vector &operator=(const vector &other) {
        if (this != &other) {
            clear();
            if (other.m_size > 0) {
                reserve(other.m_size);
                for (uint32 i = 0; i < other.m_size; i++) {
                    m_data[i] = other.m_data[i];
                }
                m_size = other.m_size;
            }
        }
        return *this;
    }

    vector &operator=(vector &&other) noexcept {
        if (this != &other) {
            if (m_data) kfree(m_data);
            m_data = other.m_data;
            m_size = other.m_size;
            m_capacity = other.m_capacity;
            other.m_data = nullptr;
            other.m_size = 0;
            other.m_capacity = 0;
        }
        return *this;
    }

    /* Capacity */
    uint32 size() const { return m_size; }
    uint32 capacity() const { return m_capacity; }
    bool empty() const { return m_size == 0; }

    void reserve(uint32 new_cap) {
        if (new_cap <= m_capacity) return;
        T *new_data = (T *)kmalloc(new_cap * sizeof(T));
        if (m_data) {
            for (uint32 i = 0; i < m_size; i++) {
                new_data[i] = m_data[i];
            }
            kfree(m_data);
        }
        m_data = new_data;
        m_capacity = new_cap;
    }

    void resize(uint32 new_size) {
        if (new_size > m_capacity) {
            reserve(new_size > m_capacity * 2 ? new_size : m_capacity * 2);
        }
        for (uint32 i = m_size; i < new_size; i++) {
            new (&m_data[i]) T();
        }
        m_size = new_size;
    }

    void clear() {
        for (uint32 i = 0; i < m_size; i++) {
            m_data[i].~T();
        }
        m_size = 0;
    }

    /* Element access */
    T &operator[](uint32 i) { return m_data[i]; }
    const T &operator[](uint32 i) const { return m_data[i]; }

    T &at(uint32 i) { return m_data[i]; }
    const T &at(uint32 i) const { return m_data[i]; }

    T &front() { return m_data[0]; }
    const T &front() const { return m_data[0]; }

    T &back() { return m_data[m_size - 1]; }
    const T &back() const { return m_data[m_size - 1]; }

    T *data() { return m_data; }
    const T *data() const { return m_data; }

    /* Modifiers */
    void push_back(const T &value) {
        if (m_size >= m_capacity) {
            reserve(m_capacity == 0 ? 8 : m_capacity * 2);
        }
        m_data[m_size++] = value;
    }

    void push_back(T &&value) {
        if (m_size >= m_capacity) {
            reserve(m_capacity == 0 ? 8 : m_capacity * 2);
        }
        m_data[m_size++] = rl::move(value);
    }

    void pop_back() {
        if (m_size > 0) {
            m_data[--m_size].~T();
        }
    }

    T *insert(T *pos, const T &value) {
        uint32 index = pos - m_data;
        if (m_size >= m_capacity) {
            reserve(m_capacity == 0 ? 8 : m_capacity * 2);
        }
        pos = m_data + index;
        for (T *p = m_data + m_size; p > pos; p--) {
            *p = *(p - 1);
        }
        *pos = value;
        m_size++;
        return pos;
    }

    T *erase(T *pos) {
        uint32 index = pos - m_data;
        for (uint32 i = index; i < m_size - 1; i++) {
            m_data[i] = m_data[i + 1];
        }
        m_size--;
        return m_data + index;
    }

    /* Iterators */
    T *begin() { return m_data; }
    T *end() { return m_data + m_size; }
    const T *begin() const { return m_data; }
    const T *end() const { return m_data + m_size; }

private:
    T *m_data;
    uint32 m_size;
    uint32 m_capacity;
};

} /* namespace rl */

#endif /* LIB_RL_VECTOR_H */
