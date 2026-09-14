#ifndef INCLUDE_STRING_H
#define INCLUDE_STRING_H

/** memcpy:
 *  Copies n bytes from src to dst.
 *
 *  @param dst  Destination buffer
 *  @param src  Source buffer
 *  @param n    Number of bytes to copy
 *  @return     A pointer to dst
 */
void *memcpy(void *dst, const void *src, unsigned int n);

/** memset:
 *  Sets n bytes of dst to the value c.
 *
 *  @param dst  Destination buffer
 *  @param c    The fill value (cast to unsigned char)
 *  @param n    Number of bytes to set
 *  @return     A pointer to dst
 */
void *memset(void *dst, int c, unsigned int n);

/** strlen:
 *  Returns the length of a string.
 *
 *  @param s  The string
 *  @return   The number of characters before the null terminator
 */
unsigned int strlen(const char *s);

/** strcmp:
 *  Compares two strings.
 *
 *  @param s1  First string
 *  @param s2  Second string
 *  @return    0 if equal, negative if s1 < s2, positive if s1 > s2
 */
int strcmp(const char *s1, const char *s2);

/** strncmp:
 *  Compares up to n characters of two strings.
 *
 *  @param s1  First string
 *  @param s2  Second string
 *  @param n   Maximum number of characters to compare
 *  @return    0 if equal, negative if s1 < s2, positive if s1 > s2
 */
int strncmp(const char *s1, const char *s2, unsigned int n);

/** strcpy:
 *  Copies the string src to dst including the null terminator.
 *
 *  @param dst  Destination buffer
 *  @param src  Source string
 *  @return     A pointer to dst
 */
char *strcpy(char *dst, const char *src);

/** strncpy:
 *  Copies at most n characters from src to dst.
 *
 *  @param dst  Destination buffer
 *  @param src  Source string
 *  @param n    Maximum number of characters to copy
 *  @return     A pointer to dst
 */
char *strncpy(char *dst, const char *src, unsigned int n);

/** strcat:
 *  Appends the string src to the end of dst.
 *
 *  @param dst  Destination buffer (must be null-terminated)
 *  @param src  Source string
 *  @return     A pointer to dst
 */
char *strcat(char *dst, const char *src);

/** strchr:
 *  Finds the first occurrence of c in s.
 *
 *  @param s  The string to search
 *  @param c  The character to find
 *  @return   A pointer to the character, or 0 if not found
 */
char *strchr(const char *s, char c);

/** strstr:
 *  Finds the first occurrence of needle in haystack.
 *
 *  @param haystack  The string to search in
 *  @param needle    The substring to find
 *  @return          A pointer to the match, or 0 if not found
 */
char *strstr(const char *haystack, const char *needle);

/** utoa:
 *  Converts an unsigned integer to a decimal string.
 *
 *  @param buf  Output buffer
 *  @param val  The value to convert
 *  @return     A pointer to the string
 */
char *utoa(char *buf, unsigned int val);

/** itoa:
 *  Converts an integer to a decimal string.
 *
 *  @param buf  Output buffer
 *  @param val  The value to convert
 *  @return     A pointer to the string
 */
char *itoa(char *buf, int val);

/** atoi:
 *  Converts a decimal string to an integer.
 *
 *  @param s  The string
 *  @return   The parsed integer, or 0 if invalid
 */
int atoi(const char *s);

#endif /* INCLUDE_STRING_H */