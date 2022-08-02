#pragma once

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <coldforce.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define LOG_CATEGORY_TEST_TCP_SERVER   1
#define LOG_CATEGORY_TEST_TCP_CLIENT   2

#define LOG_NAME_TEST_TCP_SERVER       "TCP-S"
#define LOG_NAME_TEST_TCP_CLIENT       "TCP-C"
