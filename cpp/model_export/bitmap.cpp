#include "bitmap.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>


void writeBMP( const Color *pixels, int32 w, int32 h, const char name[] ){
  char file[128];
  sprintf( file, "%s.bmp", name );
  FILE *fp = fopen( file, "wb" );

  bmp_file_magic_t magic;
  magic.num0 = 'B';
  magic.num1 = 'M';
  fwrite( &magic, 2, 1, fp ); //hard coding 2 bytes, (our structure isn't packed).

  bmp_file_header_t fileheader;
  fileheader.filesize = w * h * sizeof(int) + 54;

  fileheader.creators[0] = 0;
  fileheader.creators[1] = 0;
  fileheader.dataoffset = 54;
  fwrite( &fileheader, sizeof(bmp_file_header_t), 1, fp );

  bmp_dib_header_t dibheader;
  dibheader.headersize   = 40;
  dibheader.width        = w;
  dibheader.height       = h;
  dibheader.numplanes    = 1;
  dibheader.bitsperpixel = 24;
  dibheader.compression  = BI_RGB;
  dibheader.datasize     = 0;
  dibheader.hpixelsper   = dibheader.vpixelsper   = 1000;
  dibheader.numpalcolors = dibheader.numimpcolors = 0;
  fwrite( &dibheader, sizeof(bmp_dib_header_t), 1, fp );

  int32 rem = 0;
  if( ( w * 3 ) & 0x03 )
    rem = 4 - ( ( w * 3 ) & 0x03 );

  for( int32 y = 0; y < h; y++ ){
    for( int32 x = 0; x < w; x++ ){
      fputc( pixels[ y * w + x ].b, fp );
      fputc( pixels[ y * w + x ].g, fp );
      fputc( pixels[ y * w + x ].r, fp );
    }
    for( int32 i = 0; i < rem; i++ )
      fputc( 0xff, fp );
  }


  fclose( fp );
}

