#pragma once

#include "utils.h"

#define FDT_BEGIN_NODE (0x00000001)
#define FDT_END_NODE (0x00000002)
#define FDT_PROP (0x00000003)
#define FDT_NOP (0x00000004)
#define FDT_END (0x00000009)

typedef struct {
  uint32_t magic;
  uint32_t totalsize;
  uint32_t off_dt_struct;
  uint32_t off_dt_strings;
  uint32_t off_mem_rsvmap;
  uint32_t version;
  uint32_t last_comp_version;
  uint32_t boot_cpuid_phys;
  uint32_t size_dt_strings;
  uint32_t size_dt_struct;
} fdt_header;

void mailbox_probe(uint64_t struct_addr, uint64_t strings_addr, int depth);
void gpio_probe(uint64_t struct_addr, uint64_t strings_addr, int depth);
void rtx3080ti_probe(uint64_t struct_addr, uint64_t strings_addr, int depth);
void default_probe(uint64_t struct_addr, uint64_t strings_addr, int depth);
int check_compatibility(uint64_t struct_addr, uint64_t strings_addr,
                        char *compatible_name);

void dtb_print(int all);
void dtb_parse(uint64_t struct_addr, uint64_t strings_addr,
               void (*callback)(uint64_t, uint64_t, int));
uint64_t ignore_current_node(uint64_t struct_addr, uint64_t strings_addr);
uint64_t print_node(uint64_t struct_addr, uint64_t strings_addr, int depth);
uint64_t print_property(uint64_t struct_addr, uint64_t strings_addr, int depth);
uint32_t dtb_read_int(uint64_t addr_ptr);
// char *dtb_read_string(uint64_t *addr_ptr);