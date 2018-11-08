#ifndef _MMU_
#define _MMU_

#ifdef __cplusplus								// If we are including to a C++
extern "C" {									// Put extern C directive wrapper around
#endif
#include <stdint.h>								// Needed for uint8_t, uint32_t, etc

#define MT_DEVICE_NGNRNE	0
#define MT_DEVICE_NGNRE		1
#define MT_DEVICE_GRE		2
#define MT_NORMAL_NC		3
#define MT_NORMAL		    4

extern uint32_t table_loaded;

void init_page_table (void);

void mmu_init (void);
uint64_t virtualmap (uint32_t phys_addr, uint8_t memattrs);

#ifdef __cplusplus								// If we are including to a C++ file
}												// Close the extern C directive wrapper
#endif

#endif
