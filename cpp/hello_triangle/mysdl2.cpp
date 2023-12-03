#include "mysdl2.h"

using namespace sdl2;

Pixels::Coord Pixels::Coord::lerp( const Coord& next, float alpha ) const {
  alpha = alpha < 0.0f ? 0.0f : alpha > 1.0f ? 1.0f : alpha;
  float dx = (float)( next.x - x ) * alpha;
  float dy = (float)( next.y - y ) * alpha;
  return Coord( x + (int)dx, y + (int)dy );
}

void Pixels::plot( int x, int y, const Pixel32& pixel ){
  if( x < 0 || x >= w || y < 0 || y >= h )
    return;
  y = inverted ? h - 1 - y : y;
  if( bpp == 24 ){
    Pixel24 *pixelOut = ( Pixel24 * )&data[ y * p + ( x * 3 ) ];
    pixelOut->r = pixel.r;
    pixelOut->g = pixel.g;
    pixelOut->b = pixel.b;
  }
  else if( bpp == 32 ){
    Pixel32 *pixelOut = ( Pixel32 * )&data[ y * p + ( x * 4 ) ];
    *pixelOut = pixel;
  }
}


void Pixels::plot( int x, int y, const Pixel24& pixel ){
  if( x < 0 || x >= w || y < 0 || y >= h )
    return;
  y = inverted ? h - 1 - y : y;
  if( bpp == 24 ){
    Pixel24 *pixelOut = ( Pixel24* )&data[ y * p + ( x * 3 ) ];
    pixelOut->r = pixel.r;
    pixelOut->g = pixel.g;
    pixelOut->b = pixel.b;
  }
  else if( bpp == 32 ){
    Pixel32 *pixelOut = ( Pixel32 * )&data[ y * p + ( x * 4 ) ];
    pixelOut->r = pixel.r;
    pixelOut->g = pixel.g;
    pixelOut->b = pixel.b;
  }
}
Pixel32 *Pixels::get32( int x, int y ){
  if( !data || bpp != 32 )
    return nullptr;
  if( x < 0 || x >= w || y < 0 || y >= h )
    return nullptr;
  return (Pixel32*)&data[ y * p + ( x * 4 ) ];
}

Pixel24* Pixels::get24( int x, int y ){
  if( !data || bpp != 24 )
    return nullptr;
  if( x < 0 || x >= w || y < 0 || y >= h )
    return nullptr;
  return (Pixel24*)&data[ y * p + ( x * 3 ) ];
}


void Pixels::clear( const Pixel32& color ){
  if( bpp == 32 ){
    for( int y = 0; y < h; y++ )
      for( int x = 0; x < w; x++ ){
        Pixel32 *pixel = (Pixel32*)&data[ y * p + ( x * 4 ) ];
        *pixel = color;
      }
  }
  if( bpp == 24 ){
    for( int y = 0; y < h; y++ )
      for( int x = 0; x < w; x++ ){
        Pixel24 *pixel = (Pixel24*)&data[ y * p + ( x * 3 ) ];
        pixel->r = color.r;
        pixel->g = color.g;
        pixel->b = color.b;
      }
  }
}

void Pixels::clear( const Pixel24& color ){
  if( bpp == 32 ){
    for( int y = 0; y < h; y++ )
      for( int x = 0; x < w; x++ ){
        Pixel32 *pixel = (Pixel32*)&data[ y * p + ( x * 4 ) ];
        *pixel = color;
      }
  }
  if( bpp == 24 ){
    for( int y = 0; y < h; y++ )
      for( int x = 0; x < w; x++ ){
        Pixel24 *pixel = (Pixel24*)&data[ y * p + ( x * 3 ) ];
        pixel->r = color.r;
        pixel->g = color.g;
        pixel->b = color.b;
      }
  }
}


Bitmap::Bitmap( int w, int h, int d, std::string name ){
  surf = SDL_CreateRGBSurface( 0, w, h, d, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 );
  if( surf )
    source = std::move(name);
}

Bitmap::Bitmap( const char *bmpFile ) {
  surf = SDL_LoadBMP( bmpFile );
  if( surf )
    source = bmpFile;
}


void Bitmap::save( const char *bmpFile ) {
  if( surf && bmpFile )
    SDL_SaveBMP( surf, bmpFile );
}

int Bitmap::width(){
  if( surf )
    return surf->w;
  return 0;
}

int Bitmap::height(){
  if( surf )
    return surf->h;
  return 0;
}

int Bitmap::depth(){
  if( surf )
    return surf->format->BitsPerPixel;
  return 0;
}

bool Bitmap::locked(){
  return surf && SDL_MUSTLOCK(surf) && surf->locked;
}

void Bitmap::lock(){
  if( !surf )
    return;

  if( SDL_MUSTLOCK(surf) && !surf->locked )
    SDL_LockSurface( surf );
  pixels.data = (Uint8 *)surf->pixels;
  pixels.bpp  = surf->format->BytesPerPixel == 3 ? 24 : surf->format->BytesPerPixel == 4 ? 32 : 0;
  pixels.w    = surf->w;
  pixels.h    = surf->h;
  pixels.p    = surf->pitch;
}

void Bitmap::unlock(){
  if( !surf )
    return;

  if( SDL_MUSTLOCK(surf) && surf->locked ){
    SDL_UnlockSurface( surf );
  }
  pixels.data = nullptr;
  pixels.w = pixels.h = pixels.p = 0;
}

void Bitmap::blit( Bitmap& dstBmp, const Rect *srcRect, Rect *dstRect ){
  if( surf && dstBmp.surf )
    SDL_BlitSurface( surf, static_cast<const SDL_Rect*>(srcRect), dstBmp.surf, static_cast<SDL_Rect*>(dstRect) );
}

Bitmap::~Bitmap(){
  unlock();
  if( surf ){
    SDL_FreeSurface( surf );
    surf = nullptr;
  }
}



bool SDL::init( Uint32 w, Uint32 h, bool borderless){
  if( SDL_Init( SDL_INIT_VIDEO ) < 0 ){
    printf( "could not initialize SDL: %s\n", SDL_GetError());
    return false;
  }
  else
    printf( "SDL initialized\n" );

  inited = true;

  win = SDL_CreateWindow( "Demo",
                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                          w, h,
                          SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | (borderless ? SDL_WINDOW_BORDERLESS : 0) );
  if( !win ){
    printf( "could not create window: %s\n", SDL_GetError() );
    return false;
  }
  ctx = SDL_GL_CreateContext( win );

  keys = SDL_GetKeyboardState( &numKeys );
  for( int i = 0; i < SDL_NUM_SCANCODES; i++ )
    keyHolds[i] = 0;

  return true;
}

void SDL::pump(){
  SDL_PumpEvents();
  SDL_Event events[8];
  int total = SDL_PeepEvents( events, 8, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT );
  for( int i = 0; i < total; i++ ){
    if( events[i].type == SDL_WINDOWEVENT && events[i].window.event == SDL_WINDOWEVENT_CLOSE )
      printf( "[X]\n");
  }

  for( int i = 0; i < SDL_NUM_SCANCODES; i++ )
    if( keys[i] > 0 )
      keyHolds[i]++;
    else
      keyHolds[i] = 0;
}

void SDL::swap(){
  SDL_GL_SwapWindow( win );
}

void SDL::term(){

  if( win ){
    SDL_DestroyWindow( win );
    win = nullptr;
  }
  if( inited ){
    SDL_Quit();
    inited = false;
  }
}

Uint32 SDL::getTicks(){ return SDL_GetTicks(); }

Uint64 SDL::getPerfCounter(){ return SDL_GetPerformanceCounter();  }

double SDL::getPerfFreq(){ return (double)SDL_GetPerformanceFrequency(); }

bool SDL::keyDown( Uint8 key ){
  if( key < numKeys )
    return keys[ SDL_GetScancodeFromKey(key) ] > 0;
  return false;
}

bool SDL::keyPress( Uint8 key ){
  if( key < numKeys )
    return keyHolds[ SDL_GetScancodeFromKey(key) ] == 1;
  return false;
}

