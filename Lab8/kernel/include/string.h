# pragma once
//#include "stdint.h"
#include "utils.h"

int strcmp(const char *p1, const char *p2);
int strncmp(const char *s1, const char *s2, unsigned n);
char *strstr(const char *s, const char *find);
char *strcpy(char *dst, const char *src);
int compare(char const *a, char const *b);
int find_substr(char const *source, char const *target, int start);
int strlen(const char *s);
char *strtok(char *s, const char delim);
char *split_last(char *str, char delim);
void strcat(char *to, const char *from);
char *strncpy(char *dst, const char *src, size_t len);

int atoi(char* str);
char* itoa(int64_t val, int base);
