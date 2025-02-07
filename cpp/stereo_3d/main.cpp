#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <string>
#include <sstream>
#include <vector>
#include <memory>
#include <functional>

#include <mygl.h>
#include <vecdefs.h>
#include <strn.h>
#include <text.h>
#include <mutex>

#include "mysdl2.h"	// https://github.com/frabbani/mysdl2
#include "camera.h"
#include "obj.h"

MYGLSTRNFUNCS(64)

#define DISP_W 1280
#define DISP_H 720

using namespace sdl2;
using namespace wavefront;

constexpr int FPS = 40;
constexpr float DT_SECS = (1.0f / float(FPS));
constexpr float DT_MS = (1000.0f / float(FPS));

SDL sdl;
MyGL *mygl = nullptr;
OBJ crate;

Camera camera;

void log(const char *str) {
  static std::mutex mut;
  std::lock_guard<std::mutex> l(mut);
  static bool first = true;
  if (first) {
    printf("********************************************************\n");
    printf("********************************************************\n");
    printf("********************************************************\n");
    first = false;
  }
  printf("%s", str);
}

struct FileData {
  std::string name;
  std::vector<uint8_t> data;
  FileData() = default;
  FileData(std::string_view fileName) {
    FILE *fp = fopen(fileName.data(), "rb");
    if (!fp) {
      printf("FileData: '%s' is not a valid file\n", fileName.data());
      return;
    }
    fseek(fp, 0, SEEK_END);
    auto size = ftell(fp);
    if (!size) {
      printf("FileData: '%s' is empty\n", fileName.data());
      fclose(fp);
      return;
    }
    name = fileName;
    data.reserve(size);
    fseek(fp, 0, SEEK_SET);
    while (!feof(fp)) {
      data.push_back(fgetc(fp));
    }
    fclose(fp);
  }
};

struct Image : public MyGL_Image {
  Image() {
    w = h = 0;
    pixels = nullptr;
  }
  Image(uint32_t w, uint32_t h) {
    w = h = 0;
    pixels = nullptr;
    if (!w || !h)
      return;
    auto image = MyGL_imageAlloc(w, h);
    w = image.w;
    h = image.h;
    pixels = image.pixels;
  }
  Image(const char *bitmapFile) {
    FileData fd(bitmapFile);
    auto image = MyGL_imageFromBMPData(fd.data.data(), fd.data.size(), fd.name.c_str());
    w = image.w;
    h = image.h;
    pixels = image.pixels;
  }

  Image(const std::vector<uint8_t> &bitmapData, const char *source = "?") {
    auto image = MyGL_imageFromBMPData(bitmapData.data(), bitmapData.size(), source);
    w = image.w;
    h = image.h;
    pixels = image.pixels;
  }

  void move(MyGL_Image *to) {
    to->w = w;
    to->h = h;
    to->pixels = pixels;
    w = h = 0;
    pixels = nullptr;
  }
  ~Image() {
    if (pixels)
      MyGL_imageFree(static_cast<MyGL_Image*>(this));
    w = h = 0;
  }

  MyGL_ROImage ro() {
    return MyGL_ROImage { .w = w, .h = h, .pixels = pixels };
  }
};

struct AsciiCharSet : public MyGL_AsciiCharSet {

  AsciiCharSet(const char *name) {
    Image(MyGL_str64Fmt("assets/fonts/%s/glyphs.bmp", name).chars).move(&imageAtlas);
    this->name = MyGL_str64(name);
    if (!imageAtlas.pixels) {
      printf("failed to load font '%s' bitmap\n", name);
      return;
    }

    numChars = 0;
    for (int i = 0; i < MYGL_NUM_ASCII_PRINTABLE_CHARS; i++) {
      chars[i].c = 0;
      chars[i].x = chars[i].y = chars[i].w = chars[i].h = 0.0f;
    }

    FILE *fp = fopen(MyGL_str64Fmt("assets/fonts/%s/glyphs.txt", name).chars, "r");
    if (!fp)
      return;
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
      if (line[0] >= ' ' && line[0] <= '~') {
        int i = numChars;
        chars[i].c = line[0];
        sscanf(&line[2], "%u %u %u %u", &chars[i].w, &chars[i].h, &chars[i].x, &chars[i].y);
//        printf("'%c' %u x %u < %u , %u >\n", chars[i].c, chars[i].w, chars[i].h,
//               chars[i].x, chars[i].y);
        numChars++;
      }
    }
    fclose(fp);
  }

  ~AsciiCharSet() {
    MyGL_imageFree(&imageAtlas);
    memset(this, 0, sizeof(MyGL_AsciiCharSet));
  }
};

struct Vertex {
  MyGL_Vec3 p;
  MyGL_Vec2 t;
};

struct GetCharParam {
  std::string str;
  size_t pos = 0;
};

char getCharCb(void *param) {
  GetCharParam *p = (GetCharParam*) param;
  if (p->pos >= p->str.length())
    return '\0';
  return p->str.c_str()[p->pos++];
}

void initGround() {
  Image texImage("assets/grass.bmp");
  MyGL_createTexture2D("grass", texImage.ro(), "rgb10a2", GL_TRUE, GL_TRUE,
  GL_TRUE);

  MyGL_VertexAttrib attribs[2];
  attribs[0].components = MYGL_XYZ;
  attribs[0].normalized = GL_FALSE;
  attribs[0].type = MYGL_VERTEX_FLOAT;
  attribs[1].components = MYGL_XY;
  attribs[1].normalized = GL_FALSE;
  attribs[1].type = MYGL_VERTEX_FLOAT;

  MyGL_createVbo("ground", 4, attribs, 2);
  Vertex *vs = (Vertex*) MyGL_vboStream("ground").data;

  vs[0].p = MyGL_vec3(-5.0f, -5.0f, 0.0f);
  vs[0].t = MyGL_vec2(-3.7f, -3.7f);

  vs[1].p = MyGL_vec3(+5.0f, -5.0f, 0.0f);
  vs[1].t = MyGL_vec2(+3.7f, -3.7f);

  vs[2].p = MyGL_vec3(+5.0f, +5.0f, 0.0f);
  vs[2].t = MyGL_vec2(+3.7f, +3.7f);

  vs[3].p = MyGL_vec3(-5.0f, +5.0f, 0.0f);
  vs[3].t = MyGL_vec2(-3.7f, +3.7f);
  MyGL_vboPush("ground");

  MyGL_createIbo("ground", 6);
  uint32_t *indices = MyGL_iboStream("ground").data;
  indices[0] = 0;
  indices[1] = 1;
  indices[2] = 2;
  indices[3] = 0;
  indices[4] = 2;
  indices[5] = 3;
  MyGL_iboPush("ground");
}

void initCrate() {
  Image texImage("assets/crate.bmp");
  MyGL_createTexture2D("crate", texImage.ro(), "rgb10a2", GL_TRUE, GL_TRUE,
  GL_TRUE);

  MyGL_VertexAttrib attribs[2];
  attribs[0].components = MYGL_XYZ;
  attribs[0].normalized = GL_FALSE;
  attribs[0].type = MYGL_VERTEX_FLOAT;
  attribs[1].components = MYGL_XY;
  attribs[1].normalized = GL_FALSE;
  attribs[1].type = MYGL_VERTEX_FLOAT;

  crate.load("assets/crate.obj");
  MyGL_createVbo("crate", crate.vertices.size(), attribs, 2);
  Vertex *vs = (Vertex*) MyGL_vboStream("crate").data;
  for (size_t i = 0; i < crate.vertices.size(); i++) {
    for (int j = 0; j < 3; j++)
      vs[i].p.f3[j] = crate.vertices[i].p.xyz[j];
    for (int j = 0; j < 2; j++)
      vs[i].t.f2[j] = crate.vertices[i].t.xy[j];
  }
  MyGL_vboPush("crate");
  MyGL_createIbo("crate", crate.indices.size());
  uint32 *indices = MyGL_iboStream("crate").data;
  for (size_t i = 0; i < crate.indices.size(); i++)
    indices[i] = crate.indices[i];
  MyGL_iboPush("crate");
}

void init() {
  printf("*** INIT ***\n");

  mygl = MyGL_initialize(log, 1, 0);

  mygl->cull.on = GL_TRUE;
  MyGL_resetCull();

  mygl->depth.on = GL_TRUE;
  MyGL_resetDepth();
  mygl->blend.on = GL_TRUE;
  MyGL_resetBlend();

  mygl->stencil.on = GL_FALSE;
  MyGL_resetStencil();

  mygl->clearColor = MyGL_vec4Scale(MyGL_vec4(0.0757f, 0.075f, 0.122f, 1.0f), 1.5);
  mygl->clearDepth = 1.0f;

  auto makeCbParam = [=](const char *filename) {
    FILE *fp = fopen(filename, "r");
    std::stringstream ss;
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
      ss << line;
    }
    GetCharParam p;
    p.str = ss.str();
    return p;
  };

  printf("---------\n");
  auto param = makeCbParam("assets/shaders/includes.glsl");
  MyGL_loadShaderLibrary(getCharCb, &param, "includes.glsl");
  printf("---------\n");
  param = makeCbParam("assets/shaders/simple.shader");
  MyGL_loadShader(getCharCb, &param, "simple.shader");
  printf("---------\n");
  param = makeCbParam("assets/shaders/alphatextured.shader");
  MyGL_loadShader(getCharCb, &param, "alphatextured.shader");
  printf("---------\n");
  param = makeCbParam("assets/shaders/textured.shader");
  MyGL_loadShader(getCharCb, &param, "textured.shader");
  printf("---------\n");
  param = makeCbParam("assets/shaders/stereo.shader");
  MyGL_loadShader(getCharCb, &param, "stereo.shader");
  printf("---------\n");

  auto loadFont = [=](const char *name) {
    AsciiCharSet charSet(name);
    MyGL_loadAsciiCharSet(dynamic_cast<MyGL_AsciiCharSet*>(&charSet),
    GL_TRUE,
                          GL_TRUE);
  };
  loadFont("lemonmilk");
  initGround();
  initCrate();

  camera.position = MyGL_vec3(0.0f, -2.0f, 1.82f);
  camera.yawSpeed = camera.pitchSpeed = 0.25f * 360.0f;
  camera.forwardSpeed = 2.2f;
  camera.strafeSpeed = 1.5f;
  camera.upSpeed = 1.25;
  camera.fov = 100.0;
  printf("************\n");

}

void step() {
  static bool pause = false;
  if (sdl.keyPress('p'))
    pause = !pause;
  if (pause)
    return;

  int yaw = 0;
  int pitch = 0;
  if (sdl.keyDown(SDLK_DOWN))
    pitch = +1;
  if (sdl.keyDown(SDLK_UP))
    pitch = -1;
  if (sdl.keyDown(SDLK_LEFT))
    yaw = +1;
  if (sdl.keyDown(SDLK_RIGHT))
    yaw = -1;

  int forward = 0;
  if (sdl.keyDown('w'))
    forward++;
  if (sdl.keyDown('s'))
    forward--;
  int side = 0;
  if (sdl.keyDown('a'))
    side--;
  if (sdl.keyDown('d'))
    side++;

  int up = 0;
  if (sdl.keyDown(' '))
    up++;
  if (sdl.keyDown('c'))
    up--;

  camera.look(yaw, pitch, DT_SECS);
  camera.move(forward, side, up, DT_SECS);
}

void drawQuad() {
  auto setup_quad = [&]() {
    auto positions = MyGL_vertexAttributeStream("Position").arr.vec4s;
    positions[0] = MyGL_vec4(-1.0f, 3.0f, -1.0f, 1.0f);
    positions[1] = MyGL_vec4(+1.0f, 3.0f, -1.0f, 1.0f);
    positions[2] = MyGL_vec4(+1.0f, 3.0f, +1.0f, 1.0f);
    positions[3] = MyGL_vec4(-1.0f, 3.0f, +1.0f, 1.0f);
  };
  mygl->primitive = MYGL_QUADS;
  mygl->numPrimitives = 1;
  setup_quad();
  MyGL_drawStreaming("Position");
}

void drawScene(MyGL_Mat4 viewMatrix) {
  mygl->material = MyGL_str64("Vertex Position and Texture");
  mygl->W_matrix = MyGL_mat4Identity;
  mygl->V_matrix = viewMatrix;
  mygl->P_matrix = camera.projectionMatrix(float(DISP_W) / float(DISP_H));
  mygl->samplers[0] = MyGL_str64("grass");
  MyGL_bindSamplers();
  MyGL_drawIndexedVbo("ground", "ground", MYGL_TRIANGLES, 6);

  mygl->W_matrix = MyGL_mat4World(MyGL_vec3(0.0, 0.0, 0.5f), MyGL_vec3R, MyGL_vec3L, MyGL_vec3U);
  mygl->samplers[0] = MyGL_str64("crate");
  MyGL_bindSamplers();
  MyGL_drawIndexedVbo("crate", "crate", MYGL_TRIANGLES, crate.indices.size());
}

void drawMyInfo(MyGL_Mat4 viewMatrix) {
  MyGL_Color color;
  /* write my info */
  mygl->material = MyGL_str64("Vertex Position and Color with Alpha Texture");
  mygl->W_matrix = MyGL_mat4Identity;
  mygl->V_matrix = viewMatrix;
  mygl->P_matrix = camera.projectionMatrix(float(DISP_W) / float(DISP_H));
  mygl->samplers[0] = MyGL_str64("lemonmilk");
  MyGL_bindSamplers();
  color.value = 0xffa0a0a0;
  mygl->primitive = MYGL_QUADS;
  mygl->numPrimitives = MyGL_streamAsciiCharSet("lemonmilk", "www.youtube.com/faisal_who", color, MyGL_vec3(-0.7f, 0.02f, 1.4), MyGL_vec2(0.07f, 0.07f), 0.01f);
  MyGL_drawStreaming("Position, Color, UV0");
  mygl->numPrimitives = MyGL_streamAsciiCharSet("lemonmilk", "github.com/frabbani", color, MyGL_vec3(-0.4f, 0.0f, 1.3), MyGL_vec2(0.07f, 0.07f), 0.01f);
  MyGL_drawStreaming("Position, Color, UV0");

}

void draw() {
  auto reset = []() {
    MyGL_resetCull();
    MyGL_resetDepth();
    MyGL_resetBlend();
    MyGL_resetStencil();
    MyGL_resetColorMask();
  };
  reset();

  mygl->frameBuffer = MyGL_str64("");
  mygl->viewPort = { 0, 0, DISP_W, DISP_H };
  MyGL_bindFbo();
  MyGL_clear(GL_TRUE, GL_TRUE, GL_TRUE);
  mygl->colorMask = { 255, 0, 0, 255 };
  auto viewMatrix = camera.stereoViewMatrix(0.065, 5.0f, true);
  drawScene(viewMatrix);
  drawMyInfo(viewMatrix);
  MyGL_clear(GL_FALSE, GL_TRUE, GL_FALSE);
  mygl->colorMask = { 0, 255, 255, 255 };
  viewMatrix = camera.stereoViewMatrix(0.065, 5.0f, false);
  drawScene(viewMatrix);
  drawMyInfo(viewMatrix);
  mygl->colorMask = { 255, 255, 255, 255 };

//  mygl->frameBuffer = MyGL_str64("");
//  mygl->viewPort = { 0, 0, DISP_W, DISP_H };
//  MyGL_bindFbo();
//  mygl->clearColor = MyGL_vec4(0.09f, 0.09f, 0.18f, 1.0f);
//  MyGL_clear(GL_TRUE, GL_TRUE, GL_TRUE);
//  drawScene(camera.viewMatrix());
//  drawMyInfo(camera.viewMatrix());

}

void term() {
  printf("*** TERM ***\n");
  Uint8 *pixels = new Uint8[DISP_W * DISP_H * 3];  // RGB

  MyGL_readPixels(0, 0, DISP_W, DISP_H, MYGL_READ_RGB, MYGL_READ_BYTE, pixels);
  for (int y = 0; y < DISP_H / 2; y++)
    for (int x = 0; x < DISP_W * 3; x++)
      std::swap(pixels[y * DISP_W * 3 + x], pixels[(DISP_H - 1 - y) * DISP_W * 3 + x]);

  SDL_Surface *surf = SDL_CreateRGBSurfaceFrom(pixels, DISP_W, DISP_H, 24, DISP_W * 3, 0x0000FF, 0x00FF00, 0xFF0000, 0);
  if (surf)
    IMG_SavePNG(surf, "screenshot.png");
  SDL_FreeSurface(surf);
  delete[] pixels;
  printf("************\n");
}

int main(int argc, char *args[]) {
  setbuf( stdout, NULL);
  if (!sdl.init( DISP_W, DISP_H, false, "Stereo 3D", true))
    return 0;

  setbuf( stdout, NULL);

  double drawTimeInSecs = 0.0;
  Uint32 drawCount = 0.0;
  double invFreq = 1.0 / sdl.getPerfFreq();

  auto start = sdl.getTicks();
  auto last = start;
  init();
  while (true) {
    sdl.pump();
    if (sdl.keyDown(SDLK_ESCAPE))
      break;

    auto now = sdl.getTicks();
    auto elapsed = now - last;

    if (elapsed >= DT_MS) {
      //printf("%d -> %d (%d)\n", last, now, elapsed);
      step();  // Running at 60fps because of swap()
      last = (now / DT_MS) * DT_MS;
    }

    Uint64 beginCount = sdl.getPerfCounter();
    draw();
    drawCount++;
    sdl.swap();
    drawTimeInSecs += (double) (sdl.getPerfCounter() - beginCount) * invFreq;
  }
  term();
  if (drawCount)
    printf("Average draw time: %f ms\n", (float) ((drawTimeInSecs * 1e3) / (double) drawCount));

  sdl.term();
  printf("goodbye!\n");
  return 0;
}
