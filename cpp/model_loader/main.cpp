#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#define _USE_MATH_DEFINES
#include <cmath>
#include <string>
#include <sstream>
#include <vector>
#include <memory>
#include <functional>
#include <mutex>

#include <public/mygl.h>
#include <public/vecdefs.h>
#include <public/strn.h>
#include <public/text.h>
#include <mysdl2.h> // https://github.com/frabbani/mysdl2

MYGLSTRNFUNCS(64)

#define DISP_W 1280
#define DISP_H 720

using namespace sdl2;
SDL sdl;
MyGL *mygl = nullptr;
uint32_t numPrimitives = 0;
uint32_t maxFrames = 0;
float yawAngle = 0.0f;
float frameTime = 1.0f;
uint32_t frames[2];

void log(const char *str) {
  static bool first = true;
  static std::mutex m;
  std::lock_guard<std::mutex> l(m);
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

struct GetCharParam {
  std::string str;
  size_t pos = 0;
};

struct Mesh {
  std::string name;
  struct Vertex {
    MyGL_Vec4 p;
    MyGL_Vec2 t;
  };
  struct Triangle {
    uint32_t i, j, k;
  };
  std::vector<Vertex> vertices;
  std::vector<Triangle> triangles;
  std::vector<std::vector<MyGL_Vec3> > animations;

  Mesh(std::string name_) {
    std::string meshFile = name_ + "/mesh.txt";
    std::string framesFile = name_ + "/frames.txt";
    FILE *fp = fopen(meshFile.c_str(), "r");
    if (!fp) {
      printf("Model: invalid mesh file '%s'\n", meshFile.c_str());
      return;
    }
    name = std::move(name_);
    size_t numVerts = 0;
    size_t numTris = 0;
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
      if ('v' == line[0] && ' ' == line[1])
        numVerts++;
      else if ('f' == line[0] && ' ' == line[1])
        numTris++;
    }
    vertices.reserve(numVerts);
    triangles.reserve(numTris);
    fseek(fp, 0, SEEK_SET);
    while (fgets(line, sizeof(line), fp)) {
      if ('v' == line[0] && ' ' == line[1]) {
        float fs[8];
        sscanf(&line[2], "%f,%f,%f %f,%f,%f %f,%f", &fs[0], &fs[1], &fs[2], &fs[3], &fs[4], &fs[5], &fs[6], &fs[7]);
        Vertex v;
        v.p = MyGL_vec4(fs[0], fs[1], fs[2], 1.0f);
        v.t = MyGL_vec2(fs[6], fs[7]);
        vertices.push_back(v);
      } else if ('f' == line[0] && ' ' == line[1]) {
        uint32_t is[3];
        sscanf(&line[2], "%u,%u,%u", &is[0], &is[1], &is[2]);
        triangles.push_back(Triangle { .i = is[0], .j = is[1], .k = is[2] });
      }

    }
    fclose(fp);
    fp = fopen(framesFile.c_str(), "r");
    if (!fp) {
      return;
    }
    int index = -1;
    int count = 0;
    while (fgets(line, sizeof(line), fp)) {
      if (0 == memcmp("frame", line, 5)) {
        index++;
        count = 0;
        std::vector<MyGL_Vec3> frameVertices;
        frameVertices.reserve(vertices.size());
        for (auto v : vertices)
          frameVertices.push_back(MyGL_vec3(v.p.x, v.p.y, v.p.z));
        animations.push_back(std::move(frameVertices));

      } else if ('v' == line[0] && ' ' == line[1] && index >= 0) {
        MyGL_Vec3 p, n;
        sscanf(&line[2], "%f,%f,%f %f,%f,%f", &p.x, &p.y, &p.z, &n.x, &n.y, &n.z);
        animations.back()[count++] = p;
      }
    }
    fclose(fp);
  }
};

char getCharCb(void *param) {
  GetCharParam *p = (GetCharParam*) param;
  if (p->pos >= p->str.length())
    return '\0';
  return p->str.c_str()[p->pos++];
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
  param = makeCbParam("assets/shaders/textured.shader");
  MyGL_loadShader(getCharCb, &param, "textured.shader");
  printf("---------\n");
  param = makeCbParam("assets/shaders/textured_animated.shader");
  MyGL_loadShader(getCharCb, &param, "textured_animated.shader");
  printf("---------\n");
  param = makeCbParam("assets/shaders/alphatextured.shader");
  MyGL_loadShader(getCharCb, &param, "alphatextured.shader");
  printf("---------\n");

  MyGL_Debug_setChatty(GL_TRUE);

  auto loadFont = [=](const char *name) {
    AsciiCharSet charSet(name);
    MyGL_loadAsciiCharSet(dynamic_cast<MyGL_AsciiCharSet*>(&charSet), GL_TRUE,
    GL_TRUE);
  };

  loadFont("quake");
  loadFont("lemonmilk");

  Mesh mesh("assets/models/ranger");
  numPrimitives = mesh.triangles.size() * 3;
  MyGL_VertexAttrib attribs[] = { { MYGL_VERTEX_FLOAT, MYGL_XYZW, GL_FALSE }, { MYGL_VERTEX_FLOAT, MYGL_XY, GL_FALSE }, };
  MyGL_createVbo("Ranger", mesh.vertices.size(), attribs, 2);
  {
    MyGL_VboStream stream = MyGL_vboStream("Ranger");
    Mesh::Vertex *verts = (Mesh::Vertex*) stream.data;
    int index = 0;
    for (auto v : mesh.vertices) {
      verts[index++] = v;
      MyGL_vboPush("Ranger");
    }
  }
  MyGL_createIbo("Ranger", mesh.triangles.size() * 3);
  {
    MyGL_IboStream stream = MyGL_iboStream("Ranger");
    int index = 0;
    for (auto t : mesh.triangles) {
      stream.data[index++] = t.i;
      stream.data[index++] = t.j;
      stream.data[index++] = t.k;
    }
    MyGL_iboPush("Ranger");
  }
  if (mesh.animations.size()) {
    maxFrames = mesh.animations.size();
    printf("# of animations / vertices per: %zu / %zu\n", mesh.animations.size(), mesh.animations.front().size());
    int i = 0;
    for (const auto &frame : mesh.animations) {
      std::string name = std::string("Ranger") + std::string("/Frame") + std::to_string(i++);
      MyGL_createTbo(name.c_str(), mesh.vertices.size(), MYGL_XYZ);
      auto stream = MyGL_tboStream(name.c_str());
      int j = 0;
      for (auto p : frame) {
        stream.data[j++] = p.x;
        stream.data[j++] = p.y;
        stream.data[j++] = p.z;
      }
      MyGL_tboPush(name.c_str());
    }
  }

  Image image("assets/models/ranger/skin0.bmp");
  MyGL_createTexture2D("Ranger/Skin0", image.ro(), "rgb10a2", GL_TRUE, GL_TRUE, GL_TRUE);

  MyGL_Debug_setChatty(GL_FALSE);

  printf("************\n");

}

void step() {
  static bool pause = false;
  if (sdl.keyPress('p'))
    pause = !pause;
  if (pause)
    return;

  yawAngle -= 0.5f;
  if (yawAngle < 0.0f)
    yawAngle += 360.0f;

  std::vector<uint32_t> frameTable;
  for (uint32_t i = 1; i < maxFrames; i++)
    frameTable.push_back(i);

  frameTime += 0.1f;
  float lerp = frameTime - int(frameTime);
  frames[0] = frameTable[int(frameTime) % frameTable.size()];
  frames[1] = frameTable[(int(frameTime) + 1) % frameTable.size()];

  auto uniform = MyGL_findUniform("Vertex Position and Texture (Animated)", "Main", "lerpValue");
  if (uniform.value && uniform.info.type == MYGL_UNIFORM_FLOAT) {
    uniform.value->floa = lerp;
  }
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
  MyGL_clear( GL_TRUE, GL_TRUE, GL_TRUE);

  /* draw model */
  auto transform = [&]() {
    MyGL_Mat4 Y = MyGL_mat4Yaw(MyGL_vec3Zero, yawAngle * M_PI / 180.0f);
    MyGL_Vec3 r = MyGL_vec3R;
    MyGL_Vec3 l = MyGL_vec3L;
    MyGL_Vec3 u = MyGL_vec3U;
    l = MyGL_vec3Rotate(l, r, 90.0f * M_PI / 180.0f);
    u = MyGL_vec3Rotate(u, r, 90.0f * M_PI / 180.0f);
    return MyGL_mat4Multiply(Y, MyGL_mat4World(MyGL_vec3(0.0f, 0.0f, 0.0f), r, l, u));
  };

  mygl->material = MyGL_str64("Vertex Position and Texture (Animated)");
  mygl->W_matrix = transform();
  mygl->V_matrix = MyGL_mat4View(MyGL_vec3(0.0f, -95.0f, 8.0f), MyGL_vec3R, MyGL_vec3L, MyGL_vec3U);
  mygl->P_matrix = MyGL_mat4Perspective((float) DISP_W / (float) DISP_H, 75.0f * 3.14159265f / 180.0f, 0.01f, 1000.0f);
  mygl->samplers[0] = MyGL_str64("Ranger/Skin0");

  mygl->samplers[1] = MyGL_str64((std::string("Ranger/Frame") + std::to_string(frames[0])).c_str());
  mygl->samplers[2] = MyGL_str64((std::string("Ranger/Frame") + std::to_string(frames[1])).c_str());
  MyGL_bindSamplers();
  MyGL_drawIndexedVbo("Ranger", "Ranger", MYGL_TRIANGLES, numPrimitives);

  /* write quake text */
  mygl->material = MyGL_str64("Vertex Position and Color with Alpha Texture");
  mygl->W_matrix = MyGL_mat4Identity;
  mygl->V_matrix = MyGL_mat4Identity;
  mygl->P_matrix = MyGL_mat4Ortho(5, 5, 0.01f, 1000.0f);
  mygl->samplers[0] = MyGL_str64("quake");
  MyGL_bindSamplers();
  MyGL_Color color = MyGL_Color { .r = 247, .g = 211, .b = 139, .a = 255 };
  mygl->primitive = MYGL_QUADS;
  mygl->numPrimitives = MyGL_streamAsciiCharSet("quake", "Q RANGER Q", color, MyGL_vec3(-1.2f, 1.0f, 1.5f), MyGL_vec2(0.3f, 0.3f * DISP_W / DISP_H), 0.0f);
  MyGL_drawStreaming("Position, Color, UV0");

  /* write my info */
  mygl->W_matrix = MyGL_mat4Identity;
  mygl->V_matrix = MyGL_mat4Identity;
  mygl->P_matrix = MyGL_mat4Ortho(5, 5, 0.01f, 1000.0f);
  mygl->samplers[0] = MyGL_str64("lemonmilk");
  MyGL_bindSamplers();
  color.value = 0xff535b95;
  mygl->primitive = MYGL_QUADS;
  mygl->numPrimitives = MyGL_streamAsciiCharSet("lemonmilk", "www.youtube.com/faisal_who", color, MyGL_vec3(0.62f, 0.35f, -2.1f), MyGL_vec2(0.07f, 0.07f), 0.01f);
  MyGL_drawStreaming("Position, Color, UV0");
  mygl->numPrimitives = MyGL_streamAsciiCharSet("lemonmilk", "github.com/frabbani", color, MyGL_vec3(0.9f, 0.0f, -2.3f), MyGL_vec2(0.07f, 0.07f), 0.01f);
  MyGL_drawStreaming("Position, Color, UV0");

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

  auto start = sdl.getTicks();
  auto last = start;
  init();
  while (true) {
    sdl.pump();
    if (sdl.keyDown(SDLK_ESCAPE))
      break;

    auto now = sdl.getTicks();
    auto elapsed = now - last;

    if (elapsed >= 20) {
      step();
      last = (now / 20) * 20;
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
