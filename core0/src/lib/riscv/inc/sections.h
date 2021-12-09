#ifndef _SECTIONS_H
#define _SECTIONS_H

#ifdef __cplusplus
extern "C" {
#endif

extern uint32_t _data_start;
extern uint32_t _data_end;
extern uint32_t _data_load_addr;

extern uint32_t _bss_start;
extern uint32_t _bss_end;

extern uint32_t __stack_top;

extern uint32_t _heap_start;
extern uint32_t _heap_size;

#ifdef __cplusplus
}
#endif

#endif /* _SECTIONS_H */
