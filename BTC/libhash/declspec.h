#pragma once

#ifdef _MSC_VER
 #ifdef BUILDING_LIBHASH
  #ifdef _DLL
   #define LIBHASH_API __declspec(dllexport)
  #else
   #define LIBHASH_API
  #endif
 #else
  #ifndef LIBHASH_STATIC
   #define LIBHASH_API __declspec(dllimport)
  #else
   #define LIBHASH_API
  #endif
 #endif
#else
 #define LIBHASH_API
#endif
