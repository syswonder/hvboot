/*
 * Copyright 2025 Syswonder
 * SPDX-License-Identifier: MulanPSL-2.0
 */
#include "acpi.h"
#include "arch.h"
#include "core.h"
#include "mem.h"
#include "generated/autoconf.h"

EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
EFI_SYSTEM_TABLE *g_st;
UINTN MEMORY_MAP_ADDR;

// Helper function to print system information
static void print_system_info(EFI_SYSTEM_TABLE *SystemTable) {
  Print(L"[INFO] runtime services addr: 0x%lx\n", SystemTable->RuntimeServices);
  Print(L"[INFO] boot services addr: 0x%lx\n", SystemTable->BootServices);
  Print(L"[INFO] config table addr: 0x%lx\n", SystemTable->ConfigurationTable);
  Print(L"[INFO] con in addr: 0x%lx\n", SystemTable->ConIn);
  Print(L"[INFO] con out addr: 0x%lx\n", SystemTable->ConOut);
}

// Helper function to print binary information
static void print_binary_info() {
  Print(L"---------------------------------------------------------------------"
        L"\n");
  Print(L"hvisor uefi packer target arch: %a\n", get_arch());
#if defined(CONFIG_ENABLE_HVISOR_BIN) 
  const UINTN hvisor_bin_addr = CONFIG_HVISOR_BIN_LOAD_ADDR;
  Print(L"Hvisor binary range: 0x%lx - 0x%lx\n", hvisor_bin_addr,
        hvisor_bin_addr + (hvisor_bin_end - hvisor_bin_start));
#endif
#if defined(CONFIG_ENABLE_VMLINUX)
  const UINTN hvisor_zone0_vmlinux_addr = CONFIG_VMLINUX_LOAD_ADDR;
  Print(L"Vmlinux binary range: 0x%lx - 0x%lx\n", hvisor_zone0_vmlinux_addr,
        hvisor_zone0_vmlinux_addr +
            (hvisor_zone0_vmlinux_end - hvisor_zone0_vmlinux_start));
#endif
#if defined(CONFIG_ENABLE_DTB)
  const UINTN hvisor_zone0_dtb_addr = CONFIG_DTB_LOAD_ADDR; 
  Print(L"DeviceTree binary range: 0x%lx - 0x%lx\n", hvisor_zone0_dtb_addr,
        hvisor_zone0_dtb_addr + (hvisor_zone0_dtb_end - hvisor_zone0_dtb_start));
#endif
#if defined(CONFIG_ENABLE_INITRD)
  const UINTN hvisor_zone0_initrd_addr = CONFIG_INITRD_LOAD_ADDR;
  Print(L"InitRamdisk binary range: 0x%lx - 0x%lx\n", hvisor_zone0_initrd_addr,
        hvisor_zone0_initrd_addr + (hvisor_zone0_initrd_end - hvisor_zone0_initrd_start));
#endif
  Print(L"---------------------------------------------------------------------"
        L"\n");
}

#if defined(CONFIG_ENABLE_HVISOR_BIN) 
// Helper function to copy hvisor binary
static void copy_hvisor_binary(EFI_SYSTEM_TABLE *SystemTable) {
  const UINTN hvisor_bin_addr = CONFIG_HVISOR_BIN_LOAD_ADDR;
  UINT32 hvisor_page = ((hvisor_bin_end - hvisor_bin_start) / EFI_PAGE_SIZE) + 1;
  EFI_STATUS status = SystemTable->BootServices->AllocatePages(AllocateAddress, EfiLoaderCode, 
    hvisor_page, (EFI_PHYSICAL_ADDRESS *)&hvisor_bin_addr);
  if (EFI_ERROR(status)) Print(L"[ERROR] Can not allocate enough memory for hvisor!\n");
  Print(L"[INFO] Hvisor binary will copy to 0x%lx, size: 0x%lx\n", hvisor_bin_addr,
        hvisor_bin_end - hvisor_bin_start);
  memcpy2((void *)hvisor_bin_addr, (void *)hvisor_bin_start,
          hvisor_bin_end - hvisor_bin_start);
  Print(L"[INFO] Hvisor binary copied\n");
  flush_dcache_area(hvisor_bin_addr, (hvisor_bin_end - hvisor_bin_start));
}
#endif

#if defined(CONFIG_ENABLE_VMLINUX)
// Helper function to copy vmlinux binary
static void copy_vmlinux_binary(EFI_SYSTEM_TABLE *SystemTable) {
  const UINTN hvisor_zone0_vmlinux_addr = CONFIG_VMLINUX_LOAD_ADDR;
  UINT32 linux_page = ((hvisor_zone0_vmlinux_end - hvisor_zone0_vmlinux_start) / EFI_PAGE_SIZE) + 1;
  EFI_STATUS status = SystemTable->BootServices->AllocatePages(AllocateAddress, EfiLoaderCode, 
    linux_page, (EFI_PHYSICAL_ADDRESS *)&hvisor_zone0_vmlinux_addr);
  if (EFI_ERROR(status)) Print(L"[ERROR] Can not allocate enough memory for linux kernel!\n");
  Print(L"[INFO] Vmlinux binary will copy to 0x%lx, size: 0x%lx\n", hvisor_zone0_vmlinux_addr,
        hvisor_zone0_vmlinux_end - hvisor_zone0_vmlinux_start);
  memcpy2((void *)hvisor_zone0_vmlinux_addr, (void *)hvisor_zone0_vmlinux_start,
          hvisor_zone0_vmlinux_end - hvisor_zone0_vmlinux_start);
  Print(L"[INFO] Vmlinux binary copied\n");
  flush_dcache_area(hvisor_zone0_vmlinux_addr, (hvisor_zone0_vmlinux_end - hvisor_zone0_vmlinux_start));
}
#endif

#if defined(CONFIG_ENABLE_DTB)
// Helper function to copy dtb binary
static void copy_dtb_binary(EFI_SYSTEM_TABLE *SystemTable) {
  const UINTN hvisor_zone0_dtb_addr = CONFIG_DTB_LOAD_ADDR;
  UINT32 dtb_page = ((hvisor_zone0_dtb_end - hvisor_zone0_dtb_start) / EFI_PAGE_SIZE) + 1;
  EFI_STATUS status = SystemTable->BootServices->AllocatePages(AllocateAddress, EfiLoaderCode, 
    dtb_page,  (EFI_PHYSICAL_ADDRESS *)&hvisor_zone0_dtb_addr);
  if (EFI_ERROR(status)) Print(L"[ERROR] Can not allocate enough memory for dtb!\n");
  Print(L"[INFO] DeviceTree will copy to 0x%lx, size: 0x%lx\n", hvisor_zone0_dtb_addr,
        hvisor_zone0_dtb_end - hvisor_zone0_dtb_start);
  memcpy2((void *)hvisor_zone0_dtb_addr, (void *)hvisor_zone0_dtb_start,
          hvisor_zone0_dtb_end - hvisor_zone0_dtb_start);
  Print(L"[INFO] DeviceTree copied\n");
  flush_dcache_area(hvisor_zone0_dtb_addr, (hvisor_zone0_dtb_end - hvisor_zone0_dtb_start));
}
#endif

#if defined(CONFIG_ENABLE_INITRD)
// Helper function to copy initrd binary
static void copy_initrd_binary(EFI_SYSTEM_TABLE *SystemTable) {
  const UINTN hvisor_zone0_initrd_addr = CONFIG_INITRD_LOAD_ADDR;
  UINT32 initrd_page = ((hvisor_zone0_initrd_end - hvisor_zone0_initrd_start) / EFI_PAGE_SIZE) + 1;
  status = SystemTable->BootServices->AllocatePages(AllocateAddress, EfiLoaderCode, 
    initrd_page, (EFI_PHYSICAL_ADDRESS *)&hvisor_zone0_initrd_addr);
  if (EFI_ERROR(status)) Print(L"[ERROR] Can not allocate enough memory for initrd!\n");
  Print(L"[INFO] Linux InitRamdisk will copy to 0x%lx, size: 0x%lx\n", hvisor_zone0_initrd_addr,
        hvisor_zone0_initrd_end - hvisor_zone0_initrd_start);
  memcpy2((void *)hvisor_zone0_initrd_addr, (void *)hvisor_zone0_initrd_start,
          hvisor_zone0_initrd_end - hvisor_zone0_initrd_start);
  Print(L"[INFO] Linux InitRamdisk copied\n");
  flush_dcache_area(hvisor_zone0_initrd_addr, (hvisor_zone0_initrd_end - hvisor_zone0_initrd_start));
}
#endif

// Helper function to jump to hvisor
#if !defined(CONFIG_DIRECT_BOOT_LINUX)
static void jump_to_hvisor(EFI_SYSTEM_TABLE *SystemTable,
                           UINTN config_addr) {
  UINTN system_table = (UINTN)SystemTable;
  const UINTN hvisor_bin_addr = CONFIG_HVISOR_BIN_LOAD_ADDR;
  void (*hvisor_entry)(UINTN, UINTN) = (void (*)(UINTN, UINTN))hvisor_bin_addr;

  print_str("[INFO] ok, ready to jump to hvisor entry...\n");
  // Due to hvisor don't parse system_table/device-tree, here system_table would
  // be ignored by hvisor
  hvisor_entry(config_addr, system_table);

  // Should never reach here
  while (1) {
  }
}
#endif

// Helper function to jump to linux
#if defined(CONFIG_DIRECT_BOOT_LINUX)
static void jump_to_linux() {
  const UINTN hvisor_zone0_vmlinux_addr = CONFIG_VMLINUX_LOAD_ADDR;
  void (*linux_entry)(UINTN) = (void (*)(UINTN))(hvisor_zone0_vmlinux_addr);
  const UINTN hvisor_zone0_dtb_addr = CONFIG_DTB_LOAD_ADDR;
  linux_entry(CONFIG_DTB_LOAD_ADDR);
  print_str("[INFO] ok, ready to jump to hvisor entry...\n");
  linux_entry(hvisor_zone0_dtb_addr);
  // Should never reach here
  while (1) {
  }
}
#endif

EFI_STATUS
EFIAPI
efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {

  EFI_STATUS status;

  // Initialize architecture abstraction layer
  arch_detect_and_init();

  print_str("\n\r");
  print_str("[INFO] arch_init done\n");

  InitializeLib(ImageHandle, SystemTable);
  Print(L"[INFO] UEFI bootloader initialized!\n");
  Print(L"[INFO] Hello! This is the UEFI bootloader of hvisor, arch = %a\n",
        get_arch());
      

  Print(L"[INFO] printing system info...\n");
  print_system_info(SystemTable);

  Print(L"[INFO] before exit boot services...\n");
  ARCH_BEFORE_EXIT_BOOT_SERVICES();

  Print(L"[INFO] printing binary info...\n");
  print_binary_info();

#if defined(CONFIG_ENABLE_HVISOR_BIN) 
  Print(L"[INFO] copying hvisor binary to 0x%lx...\n", CONFIG_HVISOR_BIN_LOAD_ADDR);
  copy_hvisor_binary(SystemTable);
#endif

#if defined(CONFIG_ENABLE_VMLINUX)
  Print(L"[INFO] copying vmlinux binary to 0x%lx...\n",
        CONFIG_VMLINUX_LOAD_ADDR);
  copy_vmlinux_binary(SystemTable);
#endif

#if defined(CONFIG_ENABLE_DTB)
  Print(L"[INFO] copying devicetree binary to 0x%lx...\n",
        CONFIG_DTB_LOAD_ADDR);
  copy_dtb_binary(SystemTable);
#endif

#if defined(CONFIG_ENABLE_INITRD)
  Print(L"[INFO] copying initramdisk binary to 0x%lx...\n",
        CONFIG_INITRD_LOAD_ADDR);
  copy_initrd_binary(SystemTable);
#endif

  Print(L"[INFO] exiting boot services...\n");
  status = exit_boot_services(ImageHandle, SystemTable);

  print_str("[INFO] exit_boot_services done\n");

#if defined(CONFIG_DIRECT_BOOT_LINUX)
  jump_to_linux();
#elif defined(CONFIG_TARGET_ARCH_AARCH64)
  jump_to_hvisor(SystemTable, MEMORY_MAP_ADDR);
#else 
  EFI_BOOT_SERVICES *g_bs = SystemTable->BootServices;
  UINTN boot_cpu_id = ARCH_GET_BOOT_CPU_ID(g_bs);
  jump_to_hvisor(SystemTable, boot_cpu_id);
#endif
  return EFI_SUCCESS;
}