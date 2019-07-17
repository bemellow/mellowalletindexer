#pragma once

#ifdef _MSC_VER
 #ifdef BUILDING_LIBMISC
  #ifdef _DLL
   #define LIBMISC_API __declspec(dllexport)
  #else
   #define LIBMISC_API
  #endif
 #else
  #ifndef LIBMISC_STATIC
   #define LIBMISC_API __declspec(dllimport)
  #else
   #define LIBMISC_API
  #endif
 #endif
#else
 #define LIBMISC_API
#endif
