#ifndef LIB_RL_STDLIB_H
#define LIB_RL_STDLIB_H

#include "types.h"

/* =========================================================
 *  type_traits / utility
 * ========================================================= */

namespace rl {

template<typename T>
struct remove_reference { typedef T type; };

template<typename T>
struct remove_reference<T&> { typedef T type; };

template<typename T>
typename remove_reference<T>::type&& move(T&& arg)
{
    return static_cast<typename remove_reference<T>::type&&>(arg);
}

template<typename T, typename U>
void swap(T& a, T& b)
{
    T tmp = a;
    a = b;
    b = tmp;
}

/* =========================================================
 *  placement new
 * ========================================================= */

inline void *operator new(uint32 size, void *ptr) { return ptr; }
inline void *operator new[](uint32 size, void *ptr) { return ptr; }

/* =========================================================
 *  unique_ptr (single object)
 * ========================================================= */

template<typename T>
class unique_ptr {
public:
    unique_ptr() : m_ptr(nullptr) {}
    explicit unique_ptr(T *p) : m_ptr(p) {}

    unique_ptr(unique_ptr&& other) noexcept : m_ptr(other.m_ptr) {
        other.m_ptr = nullptr;
    }

    unique_ptr& operator=(unique_ptr&& other) noexcept {
        if (this != &other) {
            reset();
            m_ptr = other.m_ptr;
            other.m_ptr = nullptr;
        }
        return *this;
    }

    unique_ptr(const unique_ptr&) = delete;
    unique_ptr& operator=(const unique_ptr&) = delete;

    ~unique_ptr() { reset(); }

    T *get() const { return m_ptr; }
    T *release() {
        T *p = m_ptr;
        m_ptr = nullptr;
        return p;
    }

    void reset(T *p = nullptr) {
        if (m_ptr != p) {
            delete m_ptr;
            m_ptr = p;
        }
    }

    T &operator*() const { return *m_ptr; }
    T *operator->() const { return m_ptr; }
    explicit operator bool() const { return m_ptr != nullptr; }

private:
    T *m_ptr;
};

/* =========================================================
 *  unique_ptr<T[]>
 * ========================================================= */

template<typename T>
class unique_ptr<T[]> {
public:
    unique_ptr() : m_ptr(nullptr) {}
    explicit unique_ptr(T *p) : m_ptr(p) {}

    unique_ptr(unique_ptr&& other) noexcept : m_ptr(other.m_ptr) {
        other.m_ptr = nullptr;
    }

    unique_ptr& operator=(unique_ptr&& other) noexcept {
        if (this != &other) {
            reset();
            m_ptr = other.m_ptr;
            other.m_ptr = nullptr;
        }
        return *this;
    }

    unique_ptr(const unique_ptr&) = delete;
    unique_ptr& operator=(const unique_ptr&) = delete;

    ~unique_ptr() { delete[] m_ptr; }

    T *get() const { return m_ptr; }
    T *release() {
        T *p = m_ptr;
        m_ptr = nullptr;
        return p;
    }

    void reset(T *p = nullptr) {
        if (m_ptr != p) {
            delete[] m_ptr;
            m_ptr = p;
        }
    }

    T &operator[](uint32 i) const { return m_ptr[i]; }
    explicit operator bool() const { return m_ptr != nullptr; }

private:
    T *m_ptr;
};

/* =========================================================
 *  shared_ptr (reference counted)
 * ========================================================= */

template<typename T>
class shared_ptr {
public:
    shared_ptr() : m_ptr(nullptr), m_count(nullptr) {}

    explicit shared_ptr(T *p) : m_ptr(p) {
        if (p) {
            m_count = (uint32*)kmalloc(sizeof(uint32));
            *m_count = 1;
        } else {
            m_count = nullptr;
        }
    }

    shared_ptr(const shared_ptr& other)
        : m_ptr(other.m_ptr), m_count(other.m_count)
    {
        if (m_count) (*m_count)++;
    }

    shared_ptr(shared_ptr&& other) noexcept
        : m_ptr(other.m_ptr), m_count(other.m_count)
    {
        other.m_ptr = nullptr;
        other.m_count = nullptr;
    }

    shared_ptr& operator=(const shared_ptr& other) {
        if (this != &other) {
            release();
            m_ptr = other.m_ptr;
            m_count = other.m_count;
            if (m_count) (*m_count)++;
        }
        return *this;
    }

    shared_ptr& operator=(shared_ptr&& other) noexcept {
        if (this != &other) {
            release();
            m_ptr = other.m_ptr;
            m_count = other.m_count;
            other.m_ptr = nullptr;
            other.m_count = nullptr;
        }
        return *this;
    }

    ~shared_ptr() { release(); }

    T *get() const { return m_ptr; }
    uint32 use_count() const { return m_count ? *m_count : 0; }

    T &operator*() const { return *m_ptr; }
    T *operator->() const { return m_ptr; }
    explicit operator bool() const { return m_ptr != nullptr; }

private:
    void release() {
        if (m_count) {
            (*m_count)--;
            if (*m_count == 0) {
                delete m_ptr;
                kfree(m_count);
            }
        }
        m_ptr = nullptr;
        m_count = nullptr;
    }

    T *m_ptr;
    uint32 *m_count;
};

} /* namespace rl */

#endif /* LIB_RL_STDLIB_H */
