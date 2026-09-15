#ifndef LIB_CPP_RUNTIME_H
#define LIB_CPP_RUNTIME_H

#ifdef __cplusplus
extern "C" {
#endif

void __libc_init_array(void);
void __libc_fini_array(void);

#ifdef __cplusplus
}
#endif

#endif /* LIB_CPP_RUNTIME_H */
