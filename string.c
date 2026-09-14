#include "string.h"

unsigned int strlen(const char *s)
{
    unsigned int len = 0;
    while (s[len] != '\0') {
        len++;
    }
    return len;
}

int strcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

int strncmp(const char *s1, const char *s2, unsigned int n)
{
    while (n > 0 && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) {
        return 0;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

char *strcpy(char *dst, const char *src)
{
    char *d = dst;
    while ((*d++ = *src++)) {
    }
    return dst;
}

char *strncpy(char *dst, const char *src, unsigned int n)
{
    unsigned int i;
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dst[i] = src[i];
    }
    for (; i < n; i++) {
        dst[i] = '\0';
    }
    return dst;
}

char *strcat(char *dst, const char *src)
{
    char *d = dst;
    while (*d) {
        d++;
    }
    while ((*d++ = *src++)) {
    }
    return dst;
}

char *strchr(const char *s, char c)
{
    while (*s) {
        if (*s == c) {
            return (char *)s;
        }
        s++;
    }
    return (c == '\0') ? (char *)s : 0;
}

char *strstr(const char *haystack, const char *needle)
{
    const char *h;
    const char *n;
    if (*needle == '\0') {
        return (char *)haystack;
    }
    while (*haystack) {
        h = haystack;
        n = needle;
        while (*n != '\0' && *h == *n) {
            h++;
            n++;
        }
        if (*n == '\0') {
            return (char *)haystack;
        }
        haystack++;
    }
    return 0;
}

char *utoa(char *buf, unsigned int val)
{
    char tmp[12];
    unsigned int i = 0;
    unsigned int j = 0;
    unsigned int rem;

    if (val == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return buf;
    }

    while (val > 0) {
        rem = val % 10;
        tmp[i++] = (char)('0' + rem);
        val /= 10;
    }

    while (i > 0) {
        buf[j++] = tmp[--i];
    }
    buf[j] = '\0';
    return buf;
}

char *itoa(char *buf, int val)
{
    if (val < 0) {
        buf[0] = '-';
        return utoa(buf + 1, (unsigned int)(-(val + 1)) + 1);
    }
    return utoa(buf, (unsigned int)val);
}

int atoi(const char *s)
{
    int result = 0;
    int sign = 1;

    while (*s == ' ' || *s == '\t') {
        s++;
    }
    if (*s == '-') {
        sign = -1;
        s++;
    } else if (*s == '+') {
        s++;
    }
    while (*s >= '0' && *s <= '9') {
        result = result * 10 + (*s - '0');
        s++;
    }
    return sign * result;
}

void *memcpy(void *dst, const void *src, unsigned int n)
{
    char *d = (char *)dst;
    const char *s = (const char *)src;
    while (n-- > 0) {
        *d++ = *s++;
    }
    return dst;
}

void *memset(void *dst, int c, unsigned int n)
{
    char *d = (char *)dst;
    while (n-- > 0) {
        *d++ = (char)c;
    }
    return dst;
}