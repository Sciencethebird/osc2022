#include "string.h"

int strcmp(const char *p1, const char *p2);
char *strstr(const char *s, const char *find);

int compare(char const *a, char const *b){
    //for(int i = 0; i<size; i++){
    //uart_puts(a);
    while(*a){
        if(*a != *b) return 0;
        if(*a == '\0' && *b == '\0') return 1; /// ????????
        a++; b++;
    }
    return 1; // 
}

int find_substr(char const *source, char const *target, int start){
    //for(int i = 0; i<size; i++){
    //uart_puts(a);
    source += start;
    while(*source){
        if(*target == '\0' && (*source == ' ' || *source == '\0')) return 1; /// ????????
        if(*source != *target) return 0;
        source++; target++;
    }
    return (*source) == (*target); // prevent "l" see as  "ls"
}

int strlen(const char *s) {
  int len = 0;
  while (s[len] != '\0') {
    len++;
  }
  return len;
}

int atoi(char* str){
  int res = 0;
  for (int i = 0; str[i] != '\0'; ++i)
    res = res * 10 + str[i] - '0';
  // return result.
  return res;
}

char* itoa(int64_t val, int base){
    static char buf[32] = {0};
    int i = 30;
    if (val == 0) {
        buf[i] = '0';
        return &buf[i];
    }

    for (; val && i; --i, val /= base)
        buf[i] = "0123456789abcdef"[val % base];

    return &buf[i + 1];
}

int strcmp(const char *p1, const char *p2) {
  const unsigned char *s1 = (const unsigned char *)p1;
  const unsigned char *s2 = (const unsigned char *)p2;
  unsigned char c1, c2;
  do {
    c1 = (unsigned char)*s1++;
    c2 = (unsigned char)*s2++;
    if (c1 == '\0') return c1 - c2;
  } while (c1 == c2);
  return c1 - c2;
}
