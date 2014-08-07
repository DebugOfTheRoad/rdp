#ifndef CONFIG_H
#define CONFIG_H
#include "../include/lint.h"
#include "../include/platform.h"
#include "../include/rdp_def.h"

#if defined (PLATFORM_OS_WINDOWS)
#ifdef _MSC_VER
#ifdef _DEBUG
#define PLATFORM_CONFIG_DEBUG
#endif
#endif
#elif  defined (PLATFORM_OS_LINUX)
#elif  defined (PLATFORM_OS_UNIX)
#endif


#ifdef PLATFORM_CONFIG_DEBUG
#define PLATFORM_CONFIG_TEST
#endif

#endif