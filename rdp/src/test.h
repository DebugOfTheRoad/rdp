#ifndef TEST_H
#define TEST_H
#include "../include/lint.h"
#include "../include/platform.h"
#include "../include/rdp_def.h"
 
#ifdef PLATFORM_CONFIG_TEST
void test_begin();
void test_end();
#endif
 
#endif