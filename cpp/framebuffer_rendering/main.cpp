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
#include <mysdl2.h> // https://github.com/frabbani/mysdl2
#include <mutex>

MYGLSTRNFUNCS(64)

#define DISP_W 1280
#define DISP_H 720

using namespace sdl2;
SDL sdl;
MyGL *mygl = nullptr;
float angle = 0.0f;

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

void init() {
  printf("*** INIT ***\n");

  mygl = MyGL_initialize(log, 1, 64 * 1024);
  mygl->viewPort.x = 0;
  mygl->viewPort.y = 0;
  mygl->viewPort.w = DISP_W;
  mygl->viewPort.h = DISP_H;
  MyGL_bindFbo();
  mygl->cull.on = GL_TRUE;
  MyGL_resetCull();
  mygl->depth.on = GL_TRUE;
  MyGL_resetDepth();
  mygl->blend.on = GL_TRUE;
  MyGL_resetBlend();
  mygl->stencil.on = GL_FALSE;
  MyGL_resetStencil();

  struct GetCharParam {
    std::string str;
    size_t pos = 0;
  };

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

  auto getCharCb = [](void *param) -> char {
    GetCharParam *p = (GetCharParam*) param;
    if (p->pos >= p->str.length())
      return '\0';
    return p->str.c_str()[p->pos++];
  };

  printf("---------\n");
  auto param = makeCbParam("assets/shaders/includes.glsl");
  MyGL_loadShaderLibrary(getCharCb, &param, "includes.glsl");
  printf("---------\n");
  param = makeCbParam("assets/shaders/textured.shader");
  MyGL_loadShader(getCharCb, &param, "textured.shader");
  printf("---------\n");

  auto loadFont = [=](const char *name) {
    AsciiCharSet charSet(name);
    MyGL_loadAsciiCharSet(dynamic_cast<MyGL_AsciiCharSet*>(&charSet), GL_TRUE,
    GL_TRUE);
  };
  loadFont("lemonmilk");

  MyGL_Debug_setChatty(GL_TRUE);

  Image image("assets/textures/alien.bmp");
  MyGL_createTexture2D("Alien Texture", image.ro(), "rgb10a2", GL_TRUE, GL_TRUE, GL_TRUE);

  MyGL_createEmptyTexture2D("color attachment", 512, 512, "rgb10a2", GL_FALSE, GL_FALSE);
  MyGL_createEmptyTexture2D("depth/stencil attachment", 512, 512, "depth24stencil8", GL_FALSE, GL_FALSE);
  MyGL_createFbo("fbo", 512, 512);
  MyGL_fboAttachColor("fbo", "color attachment");
  MyGL_fboAttachDepthStencil("fbo", "depth/stencil attachment");
  MyGL_finalizeFbo("fbo");

  MyGL_Debug_setChatty(GL_FALSE);

  printf("************\n");

}

void step() {
  static bool pause = false;
  if (sdl.keyPress('p'))
    pause = !pause;
  if (pause)
    return;
  angle = fmodf(angle + 1.0f, 360.0f);
}

void drawFboQuad() {
  mygl->samplers[0] = MyGL_str64("color attachment");
  MyGL_bindSamplers();

  mygl->material = MyGL_str64("Vertex Position and Texture");
  mygl->W_matrix = MyGL_mat4Multiply(MyGL_mat4RotateAxis(MyGL_vec3Zero, angle * M_PI / 180.0f, 1), MyGL_mat4Scale(MyGL_vec3Zero, 0.77f, 0.77f, 0.77f));
  mygl->V_matrix = MyGL_mat4Identity;
  mygl->P_matrix = MyGL_mat4Ortho(8, 5, 0.01f, 1000.0f);

  mygl->primitive = MYGL_QUADS;
  mygl->numPrimitives = 1;

  MyGL_VertexAttributeStream vs = MyGL_vertexAttributeStream("Position");
  MyGL_VertexAttributeStream ts = MyGL_vertexAttributeStream("UV0");

  float x = -2.0f;
  float y = 1.0f;
  float z = -2.0f;
  float w = 4.0f;
  float h = 4.0f;

  vs.arr.vec4s[0] = MyGL_vec4(x, y, z, 1.0f);
  vs.arr.vec4s[1] = MyGL_vec4(x + w, y, z, 1.0f);
  vs.arr.vec4s[2] = MyGL_vec4(x + w, y, z + h, 1.0f);
  vs.arr.vec4s[3] = MyGL_vec4(x, y, z + h, 1.0f);

  ts.arr.vec4s[0] = MyGL_vec4(0.0f, 0.0f, 0.0f, 0.0f);
  ts.arr.vec4s[1] = MyGL_vec4(1.0f, 0.0f, 0.0f, 0.0f);
  ts.arr.vec4s[2] = MyGL_vec4(1.0f, 1.0f, 0.0f, 0.0f);
  ts.arr.vec4s[3] = MyGL_vec4(0.0f, 1.0f, 0.0f, 0.0f);

  MyGL_drawStreaming("Position, UV0");

}

void drawInfo() {
  /* write my info */
  MyGL_Color color;
  mygl->W_matrix = MyGL_mat4Identity;
  mygl->V_matrix = MyGL_mat4Identity;
  mygl->P_matrix = MyGL_mat4Ortho(5, 5, 0.01f, 1000.0f);
  mygl->material = MyGL_str64("Vertex Position and Color with Alpha Texture");
  mygl->samplers[0] = MyGL_str64("lemonmilk");
  MyGL_bindSamplers();
  color.value = 0xffebd263;
  mygl->primitive = MYGL_QUADS;
  mygl->numPrimitives = MyGL_streamAsciiCharSet("lemonmilk", "www.youtube.com/faisal_who", color, MyGL_vec3(0.62f, 0.35f, -2.1f), MyGL_vec2(0.07f, 0.07f), 0.011f);
  MyGL_drawStreaming("Position, Color, UV0");
  mygl->numPrimitives = MyGL_streamAsciiCharSet("lemonmilk", "github.com/frabbani", color, MyGL_vec3(0.9f, 0.0f, -2.3f), MyGL_vec2(0.07f, 0.07f), 0.011f);
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
  auto drawTriangle = [] {
    mygl->primitive = MYGL_TRIANGLES;
    mygl->numPrimitives = 1;

    MyGL_VertexAttributeStream vs = MyGL_vertexAttributeStream("Position");
    MyGL_VertexAttributeStream ts = MyGL_vertexAttributeStream("UV0");

    vs.arr.vec4s[0] = MyGL_vec4(-0.5f, 0.0f, -0.5f, 1.0f);
    vs.arr.vec4s[1] = MyGL_vec4(+0.5f, 0.0f, -0.5f, 1.0f);
    vs.arr.vec4s[2] = MyGL_vec4(0.0f, 0.0f, +0.5f, 1.0f);

    ts.arr.vec4s[0] = MyGL_vec4(0.0f, 0.0f, 0.0f, 0.0f);
    ts.arr.vec4s[1] = MyGL_vec4(1.0f, 0.0f, 0.0f, 0.0f);
    ts.arr.vec4s[2] = MyGL_vec4(0.5f, 1.0f, 0.0f, 0.0f);
    MyGL_drawStreaming("Position, UV0");
  };

  // Render to FBO
  mygl->frameBuffer = MyGL_str64("fbo");
  mygl->viewPort = { 0, 0, 512, 512 };
  MyGL_bindFbo();
  mygl->clearColor = MyGL_vec4(1.0f, 0.0f, 0.0f, 1.0f);
  mygl->clearDepth = 1.0f;
  MyGL_clear(GL_TRUE, GL_TRUE, GL_TRUE);
  mygl->material = MyGL_str64("Vertex Position and Texture");
  mygl->W_matrix = MyGL_mat4Multiply(MyGL_mat4World(MyGL_vec3(0.0f, 1.5f, 0.0f), MyGL_vec3R, MyGL_vec3L, MyGL_vec3U), MyGL_mat4RotateAxis(MyGL_vec3Zero, -angle * M_PI / 180.0f, 1));
  mygl->V_matrix = MyGL_mat4Identity;
  mygl->P_matrix = MyGL_mat4Perspective(1.0f, 75.0f * 3.14159265f / 180.0f, 0.01f, 1000.0f);
  mygl->samplers[0] = MyGL_str64("Alien Texture");
  MyGL_bindSamplers();
  drawTriangle();

  // Draw FBO to screen
  mygl->frameBuffer = MyGL_str64("");
  mygl->viewPort = { 0, 0, DISP_W, DISP_H };
  MyGL_bindFbo();
  mygl->clearColor = MyGL_vec4Scale(MyGL_vec4(7.0f, 15.0f, 33.0f, 255.0f), 1.0 / 255.0f);
  MyGL_clear(GL_TRUE, GL_TRUE, GL_TRUE);
  drawFboQuad();

  // My Info
  drawInfo();
}

void term() {
  printf("*** TERM ***\n");
  printf("************\n");
}

int main(int argc, char *args[]) {
  setbuf( stdout, NULL);
  if (!sdl.init( DISP_W, DISP_H, false, true))
    return 0;

  setbuf( stdout, NULL);

  double drawTimeInSecs = 0.0;
  Uint32 drawCount = 0.0;
  double invFreq = 1.0 / sdl.getPerfFreq();

  //auto start = sdl.getTicks();
  //auto last = start;
  init();
  while (true) {
    sdl.pump();
    if (sdl.keyDown(SDLK_ESCAPE))
      break;

    //auto now = sdl.getTicks();
    //auto elapsed = now - last;

    //if (elapsed >= 10) {
    //printf("%d -> %d (%d)\n", last, now, elapsed - 30);
    step();  // Running at 60fps because of swap()
    //  last = (now / 10) * 10;
    //}
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
