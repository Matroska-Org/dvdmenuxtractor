//#define CONFIG_STATIC
#define CONFIG_UNICODE
#define CONFIG_MULTITHREAD
#define CONFIG_MSVCRT

//-----------
// failsafes

#if !defined(ARM) || defined(TARGET_SYMBIAN)
#undef CONFIG_WMMX
#endif

#if !defined(IX86) || defined(TARGET_SYMBIAN)
#undef CONFIG_MMX
#endif

#if !defined(CONFIG_UNICODE) && (defined(TARGET_WINCE) || defined(TARGET_SYMBIAN))
#define CONFIG_UNICODE
#endif

#if defined(CONFIG_MULTITHREAD) && (defined(TARGET_SYMBIAN) || defined(TARGET_PALMOS))
#undef CONFIG_MULTITHREAD
#endif

#if defined(CONFIG_UNICODE) && (defined(TARGET_PALMOS) || defined(TARGET_LINUX))
#undef CONFIG_UNICODE
#endif

#if !defined(CONFIG_STATIC) && defined(TARGET_PALMOS) && defined(IX86)
#define CONFIG_STATIC
#endif
