#ifndef _RISCV_BITS_H
#define _RISCV_BITS_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __riscv64
# define SLL32    sllw
# define STORE    sd
# define LOAD     ld
# define LWU      lwu
# define LOG_REGBYTES 3
#else
# define SLL32    sll
# define STORE    sw
# define LOAD     lw
# define LWU      lw
# define LOG_REGBYTES 2
#endif
#define REGBYTES (1 << LOG_REGBYTES)


#ifdef __cplusplus
}
#endif

#endif
