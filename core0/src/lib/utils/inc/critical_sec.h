#ifndef CRITICAL_SECTION_H
#define CRITICAL_SECTION_H

/**
 * @brief This function is to entry cpu critical region.
 *
 */
void cpu_critical_enter(void);

/**
 * @brief This function is to exit cpu critical region.
 *
 */
void cpu_critical_exit(void);

#endif
