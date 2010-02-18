#ifndef _MSC_VER
#error "Use this header only with Microsoft Visual C++ compilers!"
#endif

#ifndef _MSC_POSIX_H_
#define _MSC_POSIX_H_

#if _MSC_VER > 1000
#pragma once
#endif

#include <sys/stat.h>
#ifndef S_ISREG
#define S_ISREG(mode) (0)
#endif
#ifndef S_ISDIR
#define S_ISDIR(mode) ((mode)&_S_IFDIR)
#endif
#ifndef S_ISCHR
#define S_ISCHR(mode) (0)
#endif
#ifndef S_ISSOCK
#define S_ISSOCK(x) (0)
#endif
#ifndef S_ISBLK
#define S_ISBLK(x) (0)
#endif

#include <stdlib.h>

#define PATH_MAX _MAX_PATH
#define strcasecmp(x, y) stricmp(x, y)
#define strncasecmp(x, y, z) strnicmp(x, y, z)

#include <windows.h>
#define dlclose(x)      FreeLibrary(x)
#define dlsym(x,y)      GetProcAddress(x,y)

#endif // _MSC_POSIX_H_
