#ifndef PLATFORM_H
#define PLATFORM_H

#if defined(WIN32) || defined(WIN64)
#define PLATFORM_OS_WINDOWS 
#elif defined(LINUX)
#define PLATFORM_OS_LINUX
#elif defined(UNIX)
#define PLATFORM_OS_UNIX
#endif

#if defined (PLATFORM_OS_WINDOWS)
#ifdef WIN32
#define PLATFORM_ARCH_X86
#else
#define PLATFORM_ARCH_X64
#endif
#elif  defined (PLATFORM_OS_LINUX)
#elif  defined (PLATFORM_OS_UNIX)
#endif


#endif
