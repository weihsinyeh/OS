#ifndef RESOURCE_H
#define RESOURCE_H
#include <stdbool.h>
extern bool resource_available[8];
void get_resources(int, int *);
void release_resources(int, int *);
bool check_resources(int count, int *resources);
#endif
