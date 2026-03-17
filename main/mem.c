
#include "mem.h"
#include "libfdt/libfdt.h"

EFI_STATUS efi_get_memory_map(EFI_SYSTEM_TABLE *SystemTable,efi_boot_memmap *map) {
    EFI_MEMORY_DESCRIPTOR *m = NULL;
    EFI_STATUS status;
    *map->map_size = 0;
    status = SystemTable->BootServices->GetMemoryMap(
        map->map_size, m, 
        map->key_ptr, 
        map->desc_size, 
        map->desc_ver
    );
    ASSERT (Status == EFI_BUFFER_TOO_SMALL);

    UINTN total_size = *map->map_size;
    UINTN request_size = total_size;
    do {
        m = (EFI_MEMORY_DESCRIPTOR *)AllocatePool(total_size);
    
        status = SystemTable->BootServices->GetMemoryMap(
            &request_size, m, 
            map->key_ptr, 
            map->desc_size, 
            map->desc_ver
        );

        if (EFI_ERROR(status)) {
            FreePool(m);
            total_size += 10 * sizeof(EFI_MEMORY_DESCRIPTOR);
            request_size = total_size;
        }
    } while (status == EFI_BUFFER_TOO_SMALL);
    // Print(L"Total Size: %d\n", total_size);
    if (EFI_ERROR(status)) {
        Print(L"[ERROR] Can not get efi memory map! status: %x\n",(UINTN)status);
        return status;
    }
    *map->map_size = request_size;
    *map->buff_size = total_size;
    *map->map = m;
    return EFI_SUCCESS;
}


EFI_STATUS efi_reget_memory_map(EFI_SYSTEM_TABLE *SystemTable,efi_boot_memmap *map) {
    EFI_STATUS status;
    UINTN buff_size = *map->buff_size;
    status = SystemTable->BootServices->GetMemoryMap(
            &buff_size, *map->map, 
            map->key_ptr, 
            map->desc_size, 
            map->desc_ver
    );
    if (EFI_ERROR(status)) {
        Print(L"[ERROR] Can not get memory map again\n");
    }
    *map->map_size = buff_size;
    return status;
}


void print_memory_map(efi_boot_memmap *map) {
    Print(L"[INFO] Map size: %x\n", *map->map_size);
    Print(L"[INFO] Map address: %lx\n", *map->map);
    Print(L"%-10s %-18s %-18s %-10s %-18s\n",
           L"Type", L"PhysicalStart", L"VirtualStart", L"Pages", L"Attribute");
    Print(L"------------------------------------------------------------------------------\n");
    UINT32 i = 0;
    UINT32 size = *map->map_size / *map->desc_size;
    if (!size) {
        Print(L"[INFO] No memory descriptors to display\n");
        return;
    }
    EFI_MEMORY_DESCRIPTOR *ptr = *map->map;
    for(i = 0; i < size; i++) {
        Print(L"%-10x 0x%-16lX 0x%-16lX %-10lx 0x%-16lX\n",
            ptr->Type,
            ptr->PhysicalStart,
            ptr->VirtualStart,
            ptr->NumberOfPages,
            ptr->Attribute);
        ptr = NextMemoryDescriptor(ptr,*map->desc_size);
    }
}


void memory_bubble_sort(efi_boot_memmap *map) {
    UINT32 size = *map->map_size / *map->desc_size;
    UINT32 i = 0,j = 0;
    BOOLEAN swap;
    if(size < 2) return; // no need for sort.
    EFI_MEMORY_DESCRIPTOR *ptr,*next_ptr;
    for(i = 0; i < size; i++) {
        swap = FALSE;
        ptr = *map->map;
        next_ptr = NextMemoryDescriptor(ptr,*map->desc_size);
        for(j = 0; j < size - i - 1; j++) {
            if(ptr->PhysicalStart > next_ptr->PhysicalStart) {
                swap = TRUE;
                memory_descriptor_swap((void *)ptr, (void *)next_ptr);
            }
            ptr = next_ptr;
            next_ptr = NextMemoryDescriptor(next_ptr, *map->desc_size);
        }
        if(!swap) break; // sort complete.
    }
}


void memory_descriptor_swap(void *o1, void* o2) {
    EFI_MEMORY_DESCRIPTOR tmp;
    EFI_MEMORY_DESCRIPTOR *ptr = &tmp;
    CopyMem(ptr, o1, sizeof(EFI_MEMORY_DESCRIPTOR));
    CopyMem(o1, o2, sizeof(EFI_MEMORY_DESCRIPTOR));
    CopyMem(o2, ptr, sizeof(EFI_MEMORY_DESCRIPTOR));
}


void virtual_mapping_algorithm(efi_boot_memmap *map, efi_boot_memmap *runtime_map) {
    UINT64 efi_virt_base = EFI_VIRT_BASE;
    EFI_MEMORY_DESCRIPTOR *in = *map->map,*prev = NULL;
    EFI_MEMORY_DESCRIPTOR *out = *runtime_map->map;
    UINT32 i = 0;
    /*
	 * To work around potential issues with the Properties Table feature
	 * introduced in UEFI 2.5, which may split PE/COFF executable images
	 * in memory into several RuntimeServicesCode and RuntimeServicesData
	 * regions, we need to preserve the relative offsets between adjacent
	 * EFI_MEMORY_RUNTIME regions with the same memory type attributes.
	 * The easiest way to find adjacent regions is to sort the memory map
	 * before traversing it.
	 */
    memory_bubble_sort(map);
    UINT32 num = *map->map_size / *map->desc_size;
    for(i = 0; i < num; i++, prev = in) {
        if (i != 0) in = NextMemoryDescriptor(in, *map->desc_size);
        if (!(in->Attribute & EFI_MEMORY_RUNTIME)) continue;
        // if (in->PhysicalStart == 0) continue;
        UINT64 paddr = in->PhysicalStart;
        UINT64 size = in->NumberOfPages * EFI_PAGE_SIZE;
        /*
		 * Make the mapping compatible with 64k pages: this allows
		 * a 4k page size kernel to kexec a 64k page size kernel and
		 * vice versa.
		 */
        if (!check_regions_are_adjacent(prev, in) 
        || !check_regions_have_compatible_memory_type_attrs(prev, in)) {
            paddr = ROUND_DOWN(in->PhysicalStart, SZ_64K);
            size += in->PhysicalStart - paddr;

            /*
			 * Avoid wasting memory on PTEs by choosing a virtual
			 * base that is compatible with section mappings if this
			 * region has the appropriate size and physical
			 * alignment. (Sections are 2 MB on 4k granule kernels)
			 */

            if (IS_ALIGNED(in->PhysicalStart, SZ_2M) && size >= SZ_2M) 
                efi_virt_base = ROUND_UP(efi_virt_base, SZ_2M);
            else
                efi_virt_base = ROUND_UP(efi_virt_base, SZ_64K);
        }

        in->VirtualStart = efi_virt_base + in->PhysicalStart - paddr;
        efi_virt_base += size;
        CopyMem((void *)out, (void *)in, sizeof(EFI_MEMORY_DESCRIPTOR));
        out = NextMemoryDescriptor(out, *map->desc_size);
        *runtime_map->map_size += *map->desc_size;
    }
}

BOOLEAN check_regions_are_adjacent(EFI_MEMORY_DESCRIPTOR *prev, EFI_MEMORY_DESCRIPTOR *ptr) {
    if (prev == NULL || ptr == NULL) return FALSE;
    UINT64 prev_end = prev->PhysicalStart + prev->NumberOfPages * EFI_PAGE_SIZE;
    return prev_end == ptr->PhysicalStart ? TRUE : FALSE;
}

BOOLEAN check_regions_have_compatible_memory_type_attrs(EFI_MEMORY_DESCRIPTOR *prev, EFI_MEMORY_DESCRIPTOR *ptr) {
    UINT64 mask = EFI_MEMORY_WB | EFI_MEMORY_WT |
					 EFI_MEMORY_WC | EFI_MEMORY_UC |
					 EFI_MEMORY_RUNTIME;
    return ((prev->Attribute ^ ptr->Attribute) & mask) == 0 ? TRUE : FALSE;
}

void create_blank_map_from_exist(efi_boot_memmap *map, efi_boot_memmap *runtime_map) {
    Print(L"[INFO] Allocating Pool, size: %d\n", *map->buff_size);
    EFI_MEMORY_DESCRIPTOR *m = (EFI_MEMORY_DESCRIPTOR *)AllocatePool(*map->buff_size);
    *runtime_map->map = m;
    *runtime_map->desc_size = *map->desc_size;
    *runtime_map->desc_ver = *map->desc_ver;
    *runtime_map->key_ptr = *map->key_ptr;
    *runtime_map->buff_size = *map->buff_size;
    *runtime_map->map_size = 0; // blank
}

EFI_STATUS add_desc_entry_to_map(efi_boot_memmap *map, EFI_MEMORY_DESCRIPTOR *desc) {
    if (*map->map_size + *map->desc_size > *map->buff_size) return EFI_BUFFER_TOO_SMALL;
    void * dst = (void *)*map->map + *map->map_size;
    void * src = (void *)desc;
    CopyMem(dst, src, *map->desc_size);
    *map->map_size += *map->desc_size;
    return EFI_SUCCESS;
}


EFI_STATUS inject_fake_system_map_desc(efi_boot_memmap *map, UINT64 target_addr) {
    EFI_MEMORY_DESCRIPTOR desc = {
        6,
        0,
        target_addr,
        0,
        1,
        EFI_MEMORY_UC | EFI_MEMORY_WC | EFI_MEMORY_WT | EFI_MEMORY_WB | EFI_MEMORY_RUNTIME
    };
    return add_desc_entry_to_map(map, &desc);
}


void init_memmap_request(efi_boot_memmap *memmap_req,
                         EFI_MEMORY_DESCRIPTOR **map_buffer_ptr,
                         UINTN *map_size, UINTN *desc_size, UINT32 *desc_ver,
                         UINTN *key_ptr, UINTN *buff_size,
                         EFI_SYSTEM_TABLE *system_table) {
  memmap_req->map = map_buffer_ptr;
  memmap_req->map_size = map_size;
  memmap_req->desc_size = desc_size;
  memmap_req->desc_ver = desc_ver;
  memmap_req->key_ptr = key_ptr;
  memmap_req->buff_size = buff_size;
  memmap_req->sys_table = (UINTN *)system_table;
  *map_size = 0;
  *buff_size = 0;
}


void flush_dcache_area(UINT64 addr, UINT64 size) {
    uint64_t tmp1, tmp2, tmp3;
    __asm__ volatile (
        "mrs  %x[tmp1], ctr_el0\n"  
        "ubfx %x[tmp2], %x[tmp1], #16, #4\n" 
        "mov  %x[tmp3], #4\n"        
        "lsl  %x[tmp3], %x[tmp3], %x[tmp2]\n"
        "add  %x[end], %x[addr], %x[size]\n"
        
        "sub  %x[mask], %x[tmp3], #1\n"
        "bic  %x[addr], %x[addr], %x[mask]\n"
        
        "1:\n"
        "dc  civac, %x[addr]\n" 
        "add  %x[addr], %x[addr], %x[tmp3]\n" 
        "cmp  %x[addr], %x[end]\n"  
        "b.cc 1b\n"                   
        "dsb  sy\n"                   
        "ic  iallu\n"             
        "isb\n"                
        : [addr] "+r" (addr),            
          [end] "=r" (tmp1),             
          [mask] "=r" (tmp2),            
          [tmp1] "=&r" (tmp1),           
          [tmp2] "=&r" (tmp2),           
          [tmp3] "=&r" (tmp3)           
        : [size] "r" (size)               
        : "memory"                   
    );
}

uint64_t get_u64_prop(void *fdt, int node, const char *name)
{
    int len;
    const fdt32_t *prop = fdt_getprop(fdt, node, name, &len);

    if (!prop || len < 8)
        return 0;

    uint64_t high = fdt32_to_cpu(prop[0]);
    uint64_t low  = fdt32_to_cpu(prop[1]);

    return (high << 32) | low;
}


uint32_t get_u32_prop(void *fdt, int node, const char *name)
{
    int len;
    const fdt32_t *prop = fdt_getprop(fdt, node, name, &len);

    if (!prop || len < 4)
        return 0;

    return fdt32_to_cpu(prop[0]);
}

void set_u64_prop(void *fdt, int node,
                                const char *name, uint64_t value)
{
    uint32_t cells[2];

    /* DTB uses big-endian */
    cells[0] = cpu_to_fdt32((uint32_t)(value >> 32));
    cells[1] = cpu_to_fdt32((uint32_t)(value & 0xffffffff));

    fdt_setprop(fdt, node, name, cells, sizeof(cells));
}

void set_u32_prop(void *fdt, int node,
                         const char *name, uint32_t value)
{
    uint32_t cell = cpu_to_fdt32(value);
    fdt_setprop(fdt, node, name, &cell, sizeof(cell));
}


EFI_STATUS redirect_memory_map_config_in_dtb(UINT64 dtb_addr,
                                       efi_boot_memmap *map)
{
    void *fdt = (void *)dtb_addr;

    /* find chosen node */
    int chosen = fdt_path_offset(fdt, "/chosen");
    if (chosen < 0)
        return EFI_ABORTED;

    uint64_t mmap_start =
        (uint64_t)(uintptr_t)(*map->map);

    uint32_t mmap_size =
        (uint32_t)(*map->map_size);

    /* ======== debug ======== */
    uint64_t old_start =
        get_u64_prop(fdt, chosen, "linux,uefi-mmap-start");
    uint32_t old_size =
        get_u32_prop(fdt, chosen, "linux,uefi-mmap-size");
    Print(L"[INFO] DTB old mmap-start: 0x%llx\n", old_start);
    Print(L"[INFO] DTB old mmap-size : 0x%x\n", old_size);
    Print(L"[INFO] New mmap-start: 0x%llx\n", mmap_start);
    Print(L"[INFO] New mmap-size : 0x%x\n", mmap_size);

    /* ======== write back ======== */
    set_u64_prop(fdt, chosen,
                 "linux,uefi-mmap-start",
                 mmap_start);

    set_u32_prop(fdt, chosen,
                 "linux,uefi-mmap-size",
                 mmap_size);
    return EFI_SUCCESS;
}