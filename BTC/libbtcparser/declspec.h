#pragma once

#ifdef _MSC_VER
 #ifdef BUILDING_LIBBTCPARSER
  #ifdef _DLL
   #define LIBBTCPARSER_API __declspec(dllexport)
  #else
   #define LIBBTCPARSER_API
  #endif
 #else
  #ifndef LIBBTCPARSER_STATIC
   #define LIBBTCPARSER_API __declspec(dllimport)
  #else
   #define LIBBTCPARSER_API
  #endif
 #endif
#else
 #define LIBBTCPARSER_API
#endif
