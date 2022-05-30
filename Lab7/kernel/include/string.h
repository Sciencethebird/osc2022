#ifndef _STRING_H
#define _STRING_H
//#include "stdint.h"
#include "utils.h"

int strcmp(const char *p1, const char *p2);
char *strstr(const char *s, const char *find);
int compare(char const *a, char const *b);
int find_substr(char const *source, char const *target, int start);
int strlen(const char *s);

int atoi(char* str);
char* itoa(int64_t val, int base);

#endif