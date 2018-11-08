#include <stdint.h>
#include "rpi-SmartStart.h"
#include "mmu.h"

/* REFERENCE FOR STAGE 1 NEXT LEVEL ENTRY */
/* ARMv8-A_Architecture_Reference_Manual_(Issue_A.a).pdf  D5-1776 */
typedef enum {
	APTABLE_NOEFFECT = 0,			// No effect
	APTABLE_NO_EL0 = 1,				// Access at EL0 not permitted, regardless of permissions in subsequent levels of lookup
	APTABLE_NO_WRITE = 2,			// Write access not permitted, at any Exception level, regardless of permissions in subsequent levels of lookup
	APTABLE_NO_WRITE_EL0_READ = 3	// Write access not permitted,at any Exception level, Read access not permitted at EL0.
} APTABLE_TYPE;

/* Basic next table level descriptor ... Figure 12.10  ... top descriptor */
/* http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.den0024a/ch12s04s01.html */
typedef union {
	struct __attribute__((__packed__)) 
	{
		uint64_t EntryType : 2;				// @0-1		Always 3 for a page table
		uint64_t _reserved2_11 : 10;		// @2-11	Set to 0
		uint64_t Address : 36;				// @12-47	36 Bits of address
		uint64_t _reserved48_58 : 11;		// @48-58	Set to 0
		uint64_t PXNTable : 1;				// @59      Never allow execution from a lower EL level 
		uint64_t XNTable : 1;				// @60		Never allow translation from a lower EL level
		APTABLE_TYPE APTable : 2;			// @61-62	AP Table control .. see enumerate options
		uint64_t NSTable : 1;				// @63		Secure state, for accesses from Non-secure state this bit is RES0 and is ignored
	};
	uint64_t Raw64;							// Raw 64bit access
} VMSAv8_64_NEXTLEVEL_DESCRIPTOR;


/* Basic Stage2 block descriptor ... Figure 12.10 ... 2nd descriptor */
/* http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.den0024a/ch12s04s01.html */
/* Attributes from section => Attribute fields in stage 2 VMSAv8-64 Block and Page descriptors */
/* https://armv8-ref.codingbelief.com/en/chapter_d4/d43_3_memory_attribute_fields_in_the_vmsav8-64_translation_table_formats_descriptors.html */

typedef union {
	struct __attribute__((__packed__))
	{
		uint64_t EntryType : 2;				// @0-1		Always 1 for a block table
		uint64_t MemAttr : 4;				// @2-5
		enum {
			STAGE2_S2AP_NOREAD_EL0 = 1,		//			No read access for EL0
			STAGE2_S2AP_NO_WRITE = 2,		//			No write access
		} S2AP : 2;							// @6-7
		enum {
			STAGE2_SH_OUTER_SHAREABLE = 2,	//			Outter shareable
			STAGE2_SH_INNER_SHAREABLE = 3,	//			Inner shareable
		} SH : 2;							// @8-9
		uint64_t AF : 1;					// @10		Accessable flag
		uint64_t _reserved11 : 1;			// @11		Set to 0
		uint64_t Address : 36;				// @12-47	36 Bits of address
		uint64_t _reserved48_51 : 4;		// @48-51	Set to 0
		uint64_t Contiguous : 1;			// @52		Contiguous
		uint64_t _reserved53 : 1;			// @53		Set to 0
		uint64_t XN : 1;					// @54		No execute if bit set
		uint64_t _reserved55_63 : 9;		// @55-63	Set to 0
	};
	uint64_t Raw64;							// Raw 64bit access
} VMSAv8_64_STAGE2_BLOCK_DESCRIPTOR;


/*--------------------------------------------------------------------------}
{					 CODE TYPE STRUCTURE COMPILE TIME CHECKS	            }
{--------------------------------------------------------------------------*/
/* If you have never seen compile time assertions it's worth google search */
/* on "Compile Time Assertions". It is part of the C11++ specification and */
/* all compilers that support the standard will have them (GCC, MSC inc)   */
/*-------------------------------------------------------------------------*/
#include <assert.h>								// Need for compile time static_assert

/* Check the code type structure size */
static_assert(sizeof(VMSAv8_64_NEXTLEVEL_DESCRIPTOR) == 0x08, "VMSAv8_64_NEXTLEVEL_DESCRIPTOR should be 0x08 bytes in size");
static_assert(sizeof(VMSAv8_64_STAGE2_BLOCK_DESCRIPTOR) == 0x08, "VMSAv8_64_STAGE2_BLOCK_DESCRIPTOR should be 0x08 bytes in size");

/* Stage1 .... 1:1 mapping to Stage2 */
static __attribute__((aligned(4096))) VMSAv8_64_NEXTLEVEL_DESCRIPTOR Stage1map1to1[512] = { 0 };

/* The Level 3 ... 1 to 1 mapping */
/* This will have 1024 entries x 2M so a full range of 2GB */
static __attribute__((aligned(4096))) VMSAv8_64_STAGE2_BLOCK_DESCRIPTOR Stage2map1to1[1024] = { 0 };


/* Stage1 ... Virtual mapping to stage 2 ... basic minimum of a single table */
static __attribute__((aligned(4096))) VMSAv8_64_NEXTLEVEL_DESCRIPTOR Stage1virtual[512] = { 0 };

/* Stage2 ... Virtual mapping to stage3 ... basic minimum of a single table */
static __attribute__((aligned(4096))) VMSAv8_64_NEXTLEVEL_DESCRIPTOR Stage2virtual[512] = { 0 };

/* Stage3 ... Virtual mapping stage3 (final) ... basic minimum of a single table */
static __attribute__((aligned(4096))) VMSAv8_64_STAGE2_BLOCK_DESCRIPTOR Stage3virtual[512] = { 0 };

uint32_t table_loaded = 0;

void init_page_table (void) {
	uint32_t base = 0;


	// initialize 1:1 mapping for TTBR0
	// 1024MB of ram memory (some belongs to the VC)
	// default: 880 MB ARM ram, 128MB VC

	/* The 21-12 entries are because that is only for 4K granual it makes it obvious to change for other granual sizes */

	/* 880Mb of ram */
	for (base = 0; base < 440; base++) {
		// Each block descriptor (2 MB)
		Stage2map1to1[base] = (VMSAv8_64_STAGE2_BLOCK_DESCRIPTOR) { .Address = (uintptr_t)base << (21-12), .AF = 1, .SH = STAGE2_SH_INNER_SHAREABLE, 
			                                                        .MemAttr = MT_NORMAL, .EntryType = 1 };
	}

	/* VC ram up to 0x3F000000 */
	for (; base < 512 - 8; base++) {
		// Each block descriptor (2 MB)
		Stage2map1to1[base] = (VMSAv8_64_STAGE2_BLOCK_DESCRIPTOR) { .Address = (uintptr_t)base << (21 - 12), .AF = 1, .MemAttr = MT_NORMAL_NC, .EntryType = 1 };
	}

	/* 16 MB peripherals at 0x3F000000 - 0x40000000*/
	for (; base < 512; base++) {
		// Each block descriptor (2 MB)
		Stage2map1to1[base] = (VMSAv8_64_STAGE2_BLOCK_DESCRIPTOR) { .Address = (uintptr_t)base << (21 - 12), .AF = 1, .MemAttr = MT_DEVICE_NGNRNE, .EntryType = 1 };
	}
	
	// 2 MB for mailboxes at 0x40000000
	// shared device, never execute
	Stage2map1to1[512] = (VMSAv8_64_STAGE2_BLOCK_DESCRIPTOR) { .Address = (uintptr_t)512 << (21 - 12), .AF = 1, .MemAttr = MT_DEVICE_NGNRNE, .EntryType = 1 };

	// unused up to 0x7FFFFFFF
	for (base = 513; base < 1024; base++) {
		Stage2map1to1[base].Raw64 = 0;
	}

	// Just 2 valid entries mapping the 2GB in stage2
	Stage1map1to1[0] = (VMSAv8_64_NEXTLEVEL_DESCRIPTOR) { .NSTable = 1, .Address = (uintptr_t)&Stage2map1to1[0] >> 12, .EntryType = 3 };
	Stage1map1to1[1] = (VMSAv8_64_NEXTLEVEL_DESCRIPTOR) { .NSTable = 1, .Address = (uintptr_t)&Stage2map1to1[512] >> 12, .EntryType = 3 };
	
	// All the rest of L2 entries are empty 
	for (uint_fast16_t i = 2; i < 512; i++) {
		Stage1map1to1[i].Raw64 = 0;
	}


	// initialize virtual mapping for TTBR1 .. basic 1 page  .. 512 entries x 4096
	// 2MB of ram memory memory  0xFFFFFFFFFFE00000 to 0xFFFFFFFFFFFFFFFF
	
	// Initially no valid entry maps in 512 entries in Stage3 virtual table we will add them via virtualmap call
	for (uint_fast16_t i = 0; i < 512; i++) {
		Stage3virtual[i].Raw64 = 0;
	}

	// Stage2 virtual has just 1 valid entry (the last) of the 512 entries pointing to the Stage3 virtual table
	Stage2virtual[511] = (VMSAv8_64_NEXTLEVEL_DESCRIPTOR) { .NSTable = 1, .Address = (uintptr_t)&Stage3virtual[0] >> 12, .EntryType = 3 };
	for (uint_fast16_t i = 0; i < 511; i++) {
		Stage2virtual[i].Raw64 = 0;
	}

	// Stage1 virtual has just 1 valid entry (the last) of 512 entries pointing to the Stage2 virtual table
	Stage1virtual[511] = (VMSAv8_64_NEXTLEVEL_DESCRIPTOR) { .NSTable = 1, .Address = (uintptr_t)&Stage2virtual[0] >> 12, .EntryType = 3 };
	for (uint_fast16_t i = 0; i < 511; i++) {
		Stage1virtual[i].Raw64 = 0;
	}

}


/**
 * Use the previous setup page translation tables
 */
void mmu_init(void)
{
    uint64_t r;
   
	/* Set the memattrs values into mair_el1*/
	asm volatile("dsb sy");
	r = ((0x00ul << (MT_DEVICE_NGNRNE * 8)) | \
		 (0x04ul << (MT_DEVICE_NGNRE * 8)) | \
		 (0x0cul << (MT_DEVICE_GRE * 8)) | \
		 (0x44ul << (MT_NORMAL_NC * 8)) | \
		 (0xfful << (MT_NORMAL * 8)));
    asm volatile ("msr mair_el1, %0" : : "r" (r));

	// Bring both tables online and execute memory barrier
	asm volatile ("msr ttbr0_el1, %0" : : "r" ((uintptr_t)&Stage1map1to1[0]));
	asm volatile ("msr ttbr1_el1, %0" : : "r" ((uintptr_t)&Stage1virtual[0]));
	asm volatile("isb");

    // Specify mapping characteristics in translate control register
    r = (0b00LL << 37) | // TBI=0, no tagging
        (0b000LL << 32)| // IPS= 32 bit ... 000 = 32bit, 001 = 36bit, 010 = 40bit
        (0b10LL << 30) | // TG1=4k ... options are 10=4KB, 01=16KB, 11=64KB ... take care differs from TG0
        (0b11LL << 28) | // SH1=3 inner ... options 00 = Non-shareable, 01 = INVALID, 10 = Outer Shareable, 11 = Inner Shareable
        (0b01LL << 26) | // ORGN1=1 write back .. options 00 = Non-cacheable, 01 = Write back cacheable, 10 = Write thru cacheable, 11 = Write Back Non-cacheable
        (0b01LL << 24) | // IRGN1=1 write back .. options 00 = Non-cacheable, 01 = Write back cacheable, 10 = Write thru cacheable, 11 = Write Back Non-cacheable
        (0b0LL  << 23) | // EPD1 ... Translation table walk disable for translations using TTBR1_EL1  0 = walk, 1 = generate fault
        (25LL   << 16) | // T1SZ=25 (512G) ... The region size is 2 POWER (64-T1SZ) bytes
        (0b00LL << 14) | // TG0=4k  ... options are 00=4KB, 01=64KB, 10=16KB,  ... take care differs from TG1
        (0b11LL << 12) | // SH0=3 inner ... .. options 00 = Non-shareable, 01 = INVALID, 10 = Outer Shareable, 11 = Inner Shareable
        (0b01LL << 10) | // ORGN0=1 write back .. options 00 = Non-cacheable, 01 = Write back cacheable, 10 = Write thru cacheable, 11 = Write Back Non-cacheable
        (0b01LL << 8)  | // IRGN0=1 write back .. options 00 = Non-cacheable, 01 = Write back cacheable, 10 = Write thru cacheable, 11 = Write Back Non-cacheable
        (0b0LL  << 7)  | // EPD0  ... Translation table walk disable for translations using TTBR0_EL1  0 = walk, 1 = generate fault
        (25LL   << 0);   // T0SZ=25 (512G)  ... The region size is 2 POWER (64-T0SZ) bytes
    asm volatile ("msr tcr_el1, %0; isb" : : "r" (r));
	
	// finally, toggle some bits in system control register to enable page translation
    asm volatile ("isb; mrs %0, sctlr_el1" : "=r" (r));
    r |= 0xC00800;     // set mandatory reserved bits
    r |= (1<<12) |     // I, Instruction cache enable. This is an enable bit for instruction caches at EL0 and EL1
           (1<<4)  |   // SA0, tack Alignment Check Enable for EL0
           (1<<3)  |   // SA, Stack Alignment Check Enable
           (1<<2)  |   // C, Data cache enable. This is an enable bit for data caches at EL0 and EL1
           (1<<1)  |   // A, Alignment check enable bit
           (1<<0);     // set M, enable MMU
    asm volatile ("msr sctlr_el1, %0; isb" : : "r" (r));

	/*release the semaphore */
	semaphore_dec(&table_loaded);
}

uint64_t virtualmap (uint32_t phys_addr, uint8_t memattrs) {
	uint64_t addr = 0;
	for (int i = 0; i < 512; i++)
	{
		if (Stage3virtual[i].Raw64 == 0) {							// Find the first vacant stage3 table slot
			uint64_t offset;
			Stage3virtual[i] = (VMSAv8_64_STAGE2_BLOCK_DESCRIPTOR) { .Address = (uintptr_t)phys_addr << (21 - 12), .AF = 1, .MemAttr = memattrs, .EntryType = 3 };
			asm volatile ("dmb sy" ::: "memory");
			offset = ((512 - i) * 4096) - 1;
			addr = 0xFFFFFFFFFFFFFFFFul;
			addr = addr - offset;
			return(addr);
		}
	}
	return (addr);													// error
}
