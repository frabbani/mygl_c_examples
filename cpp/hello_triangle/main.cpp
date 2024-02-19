
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <string>
#include <sstream>
#include <vector>
#include <memory>
#include <functional>
#include <mutex>

#include <mygl/public/mygl.h>
#include <mygl/public/vecdefs.h>
#include <mygl/public/strn.h>
#include <mygl/public/text.h>

MYGLSTRNFUNCS(64)

#include "mysdl2.h"

#define DISP_W 1280
#define DISP_H 720

using namespace sdl2;
SDL sdl;
MyGL *mygl = nullptr;

void log( const char *str ){
  static bool first = true;
  static std::mutex m;
  std::lock_guard<std::mutex> l(m);
  if( first ){
    printf( "********************************************************\n");
    printf( "********************************************************\n");
    printf( "********************************************************\n");
    first = false;
  }
  printf( "%s", str );
}


struct Image : public MyGL_Image{
  Image(){
    w = h = 0;
    pixels = nullptr;
  }
  Image( uint32_t w, uint32_t h ){
    w = h = 0;
    pixels = nullptr;
    if( !w || !h )
      return;
    auto image = MyGL_imageAlloc( w, h );
    w = image.w;
    h = image.h;
    pixels = image.pixels;
  }
  Image( const char *bitmapFile ){
    w = h = 0;
    pixels = nullptr;
    sdl2::Bitmap bmp( bitmapFile );
    if( !bmp.surf ){
      printf( "Image::Image - failed to load bitmap '%s'\n", bitmapFile );
      return;
    }
    bmp.lock();
    MyGL_Image image = MyGL_imageAlloc( bmp.pixels.w, bmp.pixels.h );
    for( uint32_t y = 0; y < image.h; y++ )
      for( uint32_t x = 0; x < image.w; x++ ){
        Pixel24 *p = bmp.pixels.get24( x, y );
        image.pixels[ y * image.w + x ].r = p->b;
        image.pixels[ y * image.w + x ].g = p->g;
        image.pixels[ y * image.w + x ].b = p->r;
        image.pixels[ y * image.w + x ].a = 255;
      }
    bmp.unlock();
    w = image.w;
    h = image.h;
    pixels = image.pixels;
  }

  void move( MyGL_Image *to ){
    to->w = w;
    to->h = h;
    to->pixels = pixels;
    w = h = 0;
    pixels = nullptr;
  }
  ~Image(){
    if( pixels )
      MyGL_imageFree( static_cast<MyGL_Image*>(this) );
    w = h = 0;
  }

  MyGL_ROImage ro(){ return MYGL_ROIMAGE(*this); }
};

struct AsciiCharSet : public MyGL_AsciiCharSet {

  AsciiCharSet( const char *name ){
    Image(  MyGL_str64Fmt( "assets/fonts/%s/glyphs.bmp", name ).chars ).move( &imageAtlas );
    this->name = MyGL_str64( name );
    if( !imageAtlas.pixels ){
      printf( "failed to load font '%s' bitmap\n", name );
      return;
    }

    numChars = 0;
    for( int i = 0; i < MYGL_NUM_ASCII_PRINTABLE_CHARS; i++ ){
      chars[i].c = 0;
      chars[i].x = chars[i].y = chars[i].w = chars[i].h = 0.0f;
    }

    FILE *fp = fopen( MyGL_str64Fmt( "assets/fonts/%s/glyphs.txt", name ).chars, "r" );
    if( !fp )
      return;
    char line[256];
    while( fgets( line, sizeof(line), fp ) ){
      if( line[0] >= ' ' && line[0] <= '~' ){
       int i = numChars;
       chars[i].c = line[0];
       sscanf( &line[2], "%u %u %u %u",
               &chars[i].w, &chars[i].h,
               &chars[i].x, &chars[i].y );
       //printf( "'%c' %u x %u < %u , %u >\n",
       //        chars[i].c,
       //        chars[i].w, chars[i].h, chars[i].x, chars[i].y );
       numChars++;
      }
    }
    fclose(fp);
  }

  ~AsciiCharSet(){
    MyGL_imageFree( &imageAtlas );
    memset( this, 0, sizeof(MyGL_AsciiCharSet) );
  }
};

struct GetCharParam {
  std::string str;
  size_t pos = 0;
};

char getCharCb( void *param ){
  GetCharParam *p = (GetCharParam *)param;
  if( p->pos >= p->str.length() )
    return '\0';
  return p->str.c_str()[ p->pos++ ];
}

void init(){
  printf( "*** INIT ***\n" );

  mygl = MyGL_initialize( log, 1, 0 );

  mygl->cull.on = GL_TRUE;
  MyGL_resetCull();

  mygl->depth.on = GL_TRUE;
  MyGL_resetDepth();
  mygl->blend.on = GL_TRUE;
  MyGL_resetBlend();

  mygl->stencil.on = GL_FALSE;
  MyGL_resetStencil();

  mygl->clearColor = MyGL_vec4( 0.0f, 0.0f, 0.16f, 1.0f );
  mygl->clearDepth = 1.0f;

  auto makeCbParam = [=]( const char *filename ){
    FILE *fp = fopen( filename, "r");
    std::stringstream ss;
    char line[256];
    while( fgets( line, sizeof(line), fp ) ){
      ss << line;
    }
    GetCharParam p;
    p.str = ss.str();
    return p;
  };

  auto param = makeCbParam( "assets/shaders/includes.glsl" );
  MyGL_loadShaderLibrary( getCharCb, &param, "includes.glsl" );
  param = makeCbParam( "assets/shaders/textured.shader" );
  MyGL_loadShader( getCharCb, &param, "textured.shader" );
  param = makeCbParam( "assets/shaders/alphatextured.shader" );
  MyGL_loadShader( getCharCb, &param, "alphatextured.shader" );

  MyGL_VertexAttrib attribs[] = {
      MYGL_VERTEX_FLOAT, MYGL_XYZW, GL_FALSE,
      MYGL_VERTEX_FLOAT, MYGL_XY,   GL_FALSE,
  };
  MyGL_createVbo( "Triangle", 3, attribs, 2 );
  MyGL_VboStream stream = MyGL_vboStream( "Triangle" );
  struct Vert{
    MyGL_Vec4 p;
    MyGL_Vec2 t;
  };
  Vert *verts = (Vert *)stream.data;
  verts[0].p = MyGL_vec4( -0.5f, 0.0f, -0.5f, 1.0f );
  verts[0].t = MyGL_vec2( 0.0f, 0.0f );

  verts[1].p = MyGL_vec4( +0.5f, 0.0f, -0.5f, 1.0f );
  verts[1].t = MyGL_vec2( 1.0f, 0.0f );

  verts[2].p = MyGL_vec4(  0.0f, 0.0f, +0.5f, 1.0f );
  verts[2].t = MyGL_vec2( 0.5f, 1.0f );
  MyGL_vboPush( "Triangle" );

  Image image( "assets/textures/alien.bmp" );
  MyGL_createTexture2D( "Alien Texture", image.ro(), "rgb10a2", GL_TRUE, GL_TRUE, GL_TRUE );

  auto loadFont = [=]( const char *name ){
    AsciiCharSet charSet( name );
    MyGL_loadAsciiCharSet( dynamic_cast<MyGL_AsciiCharSet*>(&charSet), GL_TRUE, GL_TRUE );
  };

  loadFont( "butterscotch" );
  loadFont( "lemonmilk" );

  printf( "************\n" );

}


void step(){
}




void draw(){
  auto reset = [](){
    MyGL_resetCull();
    MyGL_resetDepth();
    MyGL_resetBlend();
    MyGL_resetStencil();
    MyGL_resetColorMask();
  };

  reset();
  MyGL_clear( GL_TRUE, GL_TRUE, GL_TRUE );


  mygl->material = MyGL_str64( "Vertex Position and Texture" );
  mygl->W_matrix = MyGL_mat4World( MyGL_vec3( 0.0f, 2.5f, 0.0f ), MyGL_vec3R, MyGL_vec3L, MyGL_vec3U );
  mygl->V_matrix = MyGL_mat4Identity;
  mygl->P_matrix = MyGL_mat4Perspective( (float)DISP_W / (float)DISP_H, 75.0f * 3.14159265f / 180.0f, 0.01f, 1000.0f );
  mygl->samplers[0] = MyGL_str64( "Alien Texture" );
  MyGL_bindSamplers();
  MyGL_drawVbo( "Triangle", MYGL_TRIANGLES, 0, 3 );


  mygl->material = MyGL_str64( "Vertex Position and Color with Alpha Texture" );
  mygl->W_matrix = MyGL_mat4Identity;
  mygl->V_matrix = MyGL_mat4Identity;
  mygl->P_matrix = MyGL_mat4Ortho( 5, 5, 0.01f, 1000.0f );
  mygl->samplers[0] = MyGL_str64( "butterscotch" );
  MyGL_bindSamplers();


  MyGL_Color color = MyGL_Color{ .r=242, .g=92, .b=130, .a=255 };
  mygl->primitive = MYGL_QUADS;
  mygl->numPrimitives = MyGL_streamAsciiCharSet( "butterscotch", "Hello,", color,
                                                 MyGL_vec3( -0.4f, 1.0f, 1.5f ), MyGL_vec2( 0.4f, 0.6f ), 0.0f );

  MyGL_drawStreaming( "Position, Color, UV0" );

  mygl->numPrimitives = MyGL_streamAsciiCharSet( "butterscotch", "Triangle!", color,
                                                 MyGL_vec3( -0.7f, 1.0f, -2.0f ), MyGL_vec2( 0.4f, 0.6f ), 0.0f );

  MyGL_drawStreaming( "Position, Color, UV0" );

  mygl->samplers[0] = MyGL_str64( "lemonmilk" );
  MyGL_bindSamplers();
  mygl->W_matrix = MyGL_mat4World( MyGL_vec3( -1.5f, 1.0f, 1.5f ), MyGL_vec3R, MyGL_vec3L , MyGL_vec3U );

  mygl->W_matrix = MyGL_mat4Identity;
  mygl->V_matrix = MyGL_mat4Identity;
  mygl->P_matrix = MyGL_mat4Ortho( 5, 5, 0.01f, 1000.0f );

  color.value = 0xffffffff;
  mygl->primitive = MYGL_QUADS;
  mygl->numPrimitives = MyGL_streamAsciiCharSet( "lemonmilk", "www.youtube.com/faisal_who", color,
                                                 MyGL_vec3( 0.9f, 1.0f, -2.1f ), MyGL_vec2( 0.07f, 0.07f ), 0.01f );
  MyGL_drawStreaming( "Position, Color, UV0" );
  mygl->numPrimitives = MyGL_streamAsciiCharSet( "lemonmilk", "github.com/frabbani", color,
                                                 MyGL_vec3( 0.9f, 0.0f, -2.3f ), MyGL_vec2( 0.07f, 0.07f ), 0.01f );
  MyGL_drawStreaming( "Position, Color, UV0" );

}


void term(){
  printf( "*** TERM ***\n" );
  printf( "************\n" );
}

int main(int argc, char* args[]) {
  setbuf( stdout, NULL );
  if( !sdl.init( DISP_W, DISP_H, false ) )
    return 0;

  setbuf( stdout, NULL );

  double drawTimeInSecs = 0.0;
  Uint32 drawCount = 0.0;
  double invFreq   = 1.0 / sdl.getPerfFreq();

  auto start = sdl.getTicks();
  auto last  = start;
  init();
  while( true ){
    sdl.pump();
    if( sdl.keyDown(SDLK_ESCAPE) )
      break;

    auto now     = sdl.getTicks();
    auto elapsed = now - last;

    if( elapsed >= 20 ){
      step();
      last = ( now / 20 ) * 20;
    }
    Uint64 beginCount = sdl.getPerfCounter();
    draw();
    drawCount++;
    sdl.swap();
    drawTimeInSecs += (double)(sdl.getPerfCounter() - beginCount) * invFreq;
  }
  term();
  if( drawCount )
    printf( "Average draw time: %f ms\n", (float)( ( drawTimeInSecs * 1e3) / (double)drawCount ) );

  sdl.term();
  printf( "goodbye!\n" );
  return 0;
}
