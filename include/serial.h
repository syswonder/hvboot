#pragma once

#include <efi.h>
#include <efilib.h>

void init_serial();
void put_char(char c);
void print_str(const char *str);
void print_hex(UINT8 n);
void print_chars(char *c, int n);
