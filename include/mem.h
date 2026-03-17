#pragma once

#include <efi.h>
#include <efilib.h>

typedef struct {
    EFI_MEMORY_DESCRIPTOR **map;
	UINTN		*map_size;
	UINTN		*desc_size;
	UINT32		*desc_ver;
	UINTN		*key_ptr;
    //buffer is allocated by caller.
	UINTN		*sys_table;
    UINTN       *buff_size;
} efi_boot_memmap;

#define EFI_VIRT_BASE 0x20000000
// #define EFI_MEMORY_UC		((UINT64)0x0000000000000001ULL)	/* uncached */
// #define EFI_MEMORY_WC		((UINT64)0x0000000000000002ULL)	/* write-coalescing */
// #define EFI_MEMORY_WT		((UINT64)0x0000000000000004ULL)	/* write-through */
// #define EFI_MEMORY_WB		((UINT64)0x0000000000000008ULL)	/* write-back */
// #define EFI_MEMORY_UCE		((UINT64)0x0000000000000010ULL)	/* uncached, exported */
// #define EFI_MEMORY_WP		((UINT64)0x0000000000001000ULL)	/* write-protect */
// #define EFI_MEMORY_RP		((UINT64)0x0000000000002000ULL)	/* read-protect */
// #define EFI_MEMORY_XP		((UINT64)0x0000000000004000ULL)	/* execute-protect */
// #define EFI_MEMORY_NV		((UINT64)0x0000000000008000ULL)	/* non-volatile */
// #define EFI_MEMORY_MORE_RELIABLE ((UINT64)0x0000000000010000ULL)	/* higher reliability */
// #define EFI_MEMORY_RO		((UINT64)0x0000000000020000ULL)	/* read-only */
// #define EFI_MEMORY_RUNTIME	((UINT64)0x8000000000000000ULL)	/* range requires runtime mapping */
// #define EFI_PAGE_SHIFT		12
// #define EFI_PAGE_SIZE		(1UL << EFI_PAGE_SHIFT)

#define ROUND_DOWN(x, y) ((x) & ~((__typeof__(x))((y) - 1)))
#define ROUND_UP(x,y) ((((x)-1) | ((__typeof__(x))((y) - 1))) + 1)
#define IS_ALIGNED(x, a) (((x) & ((__typeof__(x))(a) - 1)) == 0)

#define SZ_64K				0x00010000
#define SZ_2M				0x00200000

EFI_STATUS efi_get_memory_map(EFI_SYSTEM_TABLE *SystemTable,efi_boot_memmap *map);
void print_memory_map(efi_boot_memmap *map);
void memory_bubble_sort(efi_boot_memmap *map);
void memory_descriptor_swap(void *o1, void* o2);
void virtual_mapping_algorithm(efi_boot_memmap *map, efi_boot_memmap *runtime_map);
BOOLEAN check_regions_are_adjacent(EFI_MEMORY_DESCRIPTOR *prev, EFI_MEMORY_DESCRIPTOR *ptr);
BOOLEAN check_regions_have_compatible_memory_type_attrs(EFI_MEMORY_DESCRIPTOR *prev, EFI_MEMORY_DESCRIPTOR *ptr);
void create_blank_map_from_exist(efi_boot_memmap *map, efi_boot_memmap *runtime_map);
EFI_STATUS efi_reget_memory_map(EFI_SYSTEM_TABLE *SystemTable,efi_boot_memmap *map);
EFI_STATUS add_desc_entry_to_map(efi_boot_memmap *map, EFI_MEMORY_DESCRIPTOR *desc);
EFI_STATUS inject_fake_system_map_desc(efi_boot_memmap *map, UINT64 target_addr);
void init_memmap_request(efi_boot_memmap *memmap_req,
                         EFI_MEMORY_DESCRIPTOR **map_buffer_ptr,
                         UINTN *map_size, UINTN *desc_size, UINT32 *desc_ver,
                         UINTN *key_ptr, UINTN *buff_size,
                         EFI_SYSTEM_TABLE *system_table);
void flush_dcache_area(UINT64 addr, UINT64 size);
uint64_t get_u64_prop(void *fdt, int node, const char *name);
uint32_t get_u32_prop(void *fdt, int node, const char *name);
void set_u64_prop(void *fdt, int node,
                                const char *name, uint64_t value);
void set_u32_prop(void *fdt, int node,
                         const char *name, uint32_t value);
EFI_STATUS redirect_memory_map_config_in_dtb(UINT64 dtb_addr,
                                       efi_boot_memmap *map);