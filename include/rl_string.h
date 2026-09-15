#ifndef LIB_RL_STRING_H
#define LIB_RL_STRING_H

#include "libcpp.h"
#include "string.h"  /* C string functions: strlen, memcpy, etc. */

namespace rl {

class string {
public:
    static const uint32 npos = 0xFFFFFFFF;

    string() : m_buf(s_sso_buf), m_size(0), m_capacity(SSO_CAPACITY), m_is_heap(false) {
        m_buf[0] = '\0';
    }

    string(const char *s) : m_buf(s_sso_buf), m_size(0), m_capacity(SSO_CAPACITY), m_is_heap(false) {
        uint32 len = strlen(s);
        if (len > SSO_CAPACITY) {
            grow(len);
        }
        memcpy(m_buf, s, len);
        m_size = len;
        m_buf[m_size] = '\0';
    }

    string(const char *s, uint32 len) : m_buf(s_sso_buf), m_size(0), m_capacity(SSO_CAPACITY), m_is_heap(false) {
        if (len > SSO_CAPACITY) {
            grow(len);
        }
        memcpy(m_buf, s, len);
        m_size = len;
        m_buf[m_size] = '\0';
    }

    string(const string &other) : m_buf(s_sso_buf), m_size(0), m_capacity(SSO_CAPACITY), m_is_heap(false) {
        if (other.m_size > SSO_CAPACITY) {
            grow(other.m_size);
        }
        memcpy(m_buf, other.m_buf, other.m_size);
        m_size = other.m_size;
        m_buf[m_size] = '\0';
    }

    string(string &&other) noexcept
        : m_buf(other.m_buf), m_size(other.m_size),
          m_capacity(other.m_capacity), m_is_heap(other.m_is_heap)
    {
        if (!other.m_is_heap) {
            m_buf = s_sso_buf;
            memcpy(m_buf, other.m_buf, other.m_size + 1);
        }
        other.m_buf = other.s_sso_buf;
        other.m_size = 0;
        other.m_capacity = SSO_CAPACITY;
        other.m_is_heap = false;
        other.m_buf[0] = '\0';
    }

    ~string() {
        if (m_is_heap) {
            kfree(m_buf);
        }
    }

    string &operator=(const string &other) {
        if (this != &other) {
            clear();
            if (other.m_size > m_capacity) {
                grow(other.m_size);
            }
            memcpy(m_buf, other.m_buf, other.m_size);
            m_size = other.m_size;
            m_buf[m_size] = '\0';
        }
        return *this;
    }

    string &operator=(string &&other) noexcept {
        if (this != &other) {
            if (m_is_heap) kfree(m_buf);
            m_buf = other.m_buf;
            m_size = other.m_size;
            m_capacity = other.m_capacity;
            m_is_heap = other.m_is_heap;
            if (!other.m_is_heap) {
                m_buf = s_sso_buf;
                memcpy(m_buf, other.m_buf, other.m_size + 1);
            }
            other.m_buf = other.s_sso_buf;
            other.m_size = 0;
            other.m_capacity = SSO_CAPACITY;
            other.m_is_heap = false;
            other.m_buf[0] = '\0';
        }
        return *this;
    }

    string &operator=(const char *s) {
        clear();
        uint32 len = strlen(s);
        if (len > m_capacity) grow(len);
        memcpy(m_buf, s, len);
        m_size = len;
        m_buf[m_size] = '\0';
        return *this;
    }

    /* Capacity */
    uint32 size() const { return m_size; }
    uint32 length() const { return m_size; }
    uint32 capacity() const { return m_capacity; }
    bool empty() const { return m_size == 0; }

    void clear() {
        m_size = 0;
        m_buf[0] = '\0';
    }

    /* Element access */
    char &operator[](uint32 i) { return m_buf[i]; }
    char operator[](uint32 i) const { return m_buf[i]; }
    char &at(uint32 i) { return m_buf[i]; }
    const char *c_str() const { return m_buf; }
    const char *data() const { return m_buf; }

    /* Modifiers */
    string &operator+=(char c) {
        if (m_size + 1 >= m_capacity) grow(m_size + 8);
        m_buf[m_size++] = c;
        m_buf[m_size] = '\0';
        return *this;
    }

    string &operator+=(const char *s) {
        uint32 len = strlen(s);
        if (m_size + len >= m_capacity) grow(m_size + len + 8);
        memcpy(m_buf + m_size, s, len);
        m_size += len;
        m_buf[m_size] = '\0';
        return *this;
    }

    string &operator+=(const string &other) {
        return operator+=(other.m_buf);
    }

    string operator+(const char *s) const {
        string result(*this);
        result += s;
        return result;
    }

    string operator+(const string &other) const {
        string result(*this);
        result += other;
        return result;
    }

    /* Comparison */
    bool operator==(const string &other) const {
        if (m_size != other.m_size) return false;
        return strcmpl(m_buf, other.m_buf) == 0;
    }

    bool operator==(const char *s) const {
        return strcmpl(m_buf, s) == 0;
    }

    bool operator!=(const string &other) const { return !(*this == other); }
    bool operator!=(const char *s) const { return !(*this == s); }

    bool operator<(const string &other) const {
        return strcmpl(m_buf, other.m_buf) < 0;
    }

    bool operator>(const string &other) const {
        return strcmpl(m_buf, other.m_buf) > 0;
    }

    /* Find */
    int find(char c, uint32 pos = 0) const {
        for (uint32 i = pos; i < m_size; i++) {
            if (m_buf[i] == c) return (int)i;
        }
        return -1;
    }

    int find(const char *s, uint32 pos = 0) const {
        uint32 slen = strlen(s);
        if (slen == 0) return (int)pos;
        for (uint32 i = pos; i + slen <= m_size; i++) {
            if (strcmpl(m_buf + i, s) == 0) return (int)i;
        }
        return -1;
    }

    string substr(uint32 pos, uint32 len = npos) const {
        if (pos >= m_size) return string();
        if (len > m_size - pos) len = m_size - pos;
        return string(m_buf + pos, len);
    }

    /* Reserve */
    void reserve(uint32 new_cap) {
        if (new_cap > m_capacity) grow(new_cap);
    }

    /* Iterator-like (for range-for) */
    char *begin() { return m_buf; }
    char *end() { return m_buf + m_size; }
    const char *begin() const { return m_buf; }
    const char *end() const { return m_buf + m_size; }

private:
    static const uint32 SSO_CAPACITY = 15;

    void grow(uint32 new_cap) {
        new_cap = (new_cap + 15) & ~15;  /* align to 16 */
        char *new_buf = (char *)kmalloc(new_cap + 1);
        if (m_size > 0) memcpy(new_buf, m_buf, m_size);
        new_buf[m_size] = '\0';
        if (m_is_heap) kfree(m_buf);
        m_buf = new_buf;
        m_capacity = new_cap;
        m_is_heap = true;
    }

    static int strcmpl(const char *a, const char *b) {
        while (*a && *a == *b) { a++; b++; }
        return (unsigned char)*a - (unsigned char)*b;
    }

    char *m_buf;
    uint32 m_size;
    uint32 m_capacity;
    bool m_is_heap;
    char s_sso_buf[SSO_CAPACITY + 1];
};

} /* namespace rl */

#endif /* LIB_RL_STRING_H */
