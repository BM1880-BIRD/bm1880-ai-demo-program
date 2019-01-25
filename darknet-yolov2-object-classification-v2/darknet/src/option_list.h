#ifndef OPTION_LIST_H
#define OPTION_LIST_H
#include "list.h"

typedef struct{
    char *key;
    char *val;
    int used;
} kvp;


int read_option(char *s, list *options);
void option_insert(list *l, char *key, char *val);
char *option_find(list *l, char *key);
float option_find_float(list *l, char *key, float def);
void option_find_float_list(list *l, char *key, float *val, int count);
float option_find_float_quiet(list *l, char *key, float def);
void option_unused(list *l);

#endif
