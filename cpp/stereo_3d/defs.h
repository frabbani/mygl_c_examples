#pragma once

#include <cinttypes>

#define SWAP( a, b ) \
{ \
  __typeof__(a) t = a; \
  a = b; \
  b = t; \
} \

#define LERP ( a, b, s ) ( (1-(s))*(a) + (s)*(b) )

#define CLAMP( a, l, h ) \
{ \
  a = a < (l) ? (l) : a > (h) ? (h) : a;  \
} \

#define SORT2( a, b ) \
{ \
    if( (b) < (a) )\
        SWAP( a, b );\
} \

#define SORT3( a, b, c )  \
{\
    if( (c) < (b) )\
        SWAP( b, c );\
    if( (c) < (a) )\
        SWAP( a, c );\
    if( (b) < (a) )\
        SWAP( a, b );\
}\


typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

int32 ssplit(char str[], char *toks[], int32 maxtoks, const char delims[]);

#pragma pack(push,1)
struct Color {
  union {
    struct {
      uint8 r, g, b;
    };
    uint8 rgb[3];
  };
  Color(uint8 r = 255, uint8 g = 0, uint8 b = 255);
  Color& lerp(Color that, float alpha);
  Color& fromHex(const char *str);  //example #80AACD or #80aacd
  static void writePPM(const Color *pixels, uint32 w, uint32 h, const char name[]);
};
#pragma pack(pop)

