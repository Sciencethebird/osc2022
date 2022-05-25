#pragma once
//#include "stdint.h"
#include <stddef.h>
#include <stdint.h>
//typedef unsigned long size_t;
//
//typedef unsigned long long int uint64_t;
//typedef signed long long int int64_t;
//
//typedef unsigned int uint32_t;
//typedef signed int int32_t;
//
//typedef unsigned short int uint16_t;
//typedef signed short int int16_t;
//
//typedef unsigned char uint8_t;
//typedef signed char int8_t;


#define kb ((uint64_t)0x400)
#define mb ((uint64_t)0x100000)
#define gb ((uint64_t)0x40000000)

int is_digit(char ch);
uint64_t hex2int(char *hex, int len);
uint64_t align_up(uint64_t addr, uint64_t alignment);
uint64_t align_up_exp(uint64_t n);
uint32_t get_value32(uint64_t addr, char endian);
uint32_t be2le(uint32_t x);
uint64_t hex_to_int64(char*);
uint8_t hex_to_int8(char);

uint64_t log2(uint64_t num);

void bp(char* msg);

void delay(int num);