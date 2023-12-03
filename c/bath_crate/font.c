#include "font.h"

#include <mygl.h>
#include <vecdefs.h>
#include <stdio.h>


void load_font( const char name[] ){
  MyGL_Image atlas = BMP_init_image(  MyGL_str64Fmt( "assets/%s/glyphs.bmp", name ).chars );
  if( !atlas.pixels ){
    printf( "failed to load font '%s' bitmap\n", name );
    return;
  }

  MyGL_AsciiCharSet char_set;

  char_set.name = MyGL_str64( name );
  char_set.imageAtlas =  atlas;

  FILE *fp = fopen( MyGL_str64Fmt( "assets/%s/glyphs.txt", name ).chars, "r" );
  if( !fp ){
    printf( "failed to load font '%s' info\n", name );
    MyGL_imageFree( &char_set.imageAtlas );
    return;
  }
  char line[256];

  for( int i = 0; i < MYGL_NUM_ASCII_PRINTABLE_CHARS; i++ ){
    char_set.chars[i].c = 0;
    char_set.chars[i].x = char_set.chars[i].y = 0.0f;
    char_set.chars[i].w = char_set.chars[i].h = 0.0f;
  }

  char_set.numChars = 0;
  while( fgets( line, sizeof(line), fp ) ){
    if( line[0] >= ' ' && line[0] <= '~' ){
      int i = char_set.numChars;
      char_set.chars[i].c = line[0];
      sscanf( &line[2], "%u %u %u %u",
              &char_set.chars[i].w, &char_set.chars[i].h,
              &char_set.chars[i].x, &char_set.chars[i].y );
      //printf( "'%c' %u x %u < %u , %u >\n",
      //        char_set.chars[i].c,
      //        char_set.chars[i].w, char_set.chars[i].h, char_set.chars[i].x, char_set.chars[i].y );
      char_set.numChars++;
    }
  }
  fclose(fp);

  MyGL_loadAsciiCharSet( &char_set, GL_TRUE, GL_TRUE );
  MyGL_imageFree( &char_set.imageAtlas );
}
