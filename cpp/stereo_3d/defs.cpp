#include "defs.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

int32 ssplit(char str[], char *toks[], int32 maxtoks, const char delims[]) {
  int32 n = 0;
  toks[n++] = strtok(str, delims);
  while (1) {
    char *ptr = strtok( NULL, delims);
    if (!ptr)
      break;
    toks[n++] = ptr;
    if (n == maxtoks)
      break;
  }
  return n;
}

Color::Color(uint8 r, uint8 g, uint8 b) {
  this->r = r;
  this->g = g;
  this->b = b;
}

Color& Color::lerp(Color that, float alpha) {
  alpha = alpha < 0.0f ? 0.0f : alpha > 1.0f ? 1.0f : alpha;
  float rf = (float) r + alpha * (float) (that.r - r);
  float gf = (float) g + alpha * (float) (that.g - g);
  float bf = (float) b + alpha * (float) (that.b - b);
  r = (uint8) rf;
  g = (uint8) gf;
  b = (uint8) bf;
  return *this;
}

Color& Color::fromHex(const char *str) {
  r = g = b = 0;
  if (str[0] == '#') {
    uint32 rgb;
    sscanf(&str[1], "%x", &rgb);
    b = rgb & 0xff;
    g = (rgb >> 8) & 0xff;
    r = (rgb >> 16) & 0xff;
  }
  return *this;
}

void Color::writePPM(const Color *pixels, uint32 w, uint32 h, const char name[]) {
  char file[128];
  sprintf(file, "%s.ppm", name);
  FILE *fp = fopen(file, "wb");
  fprintf(fp, "P6\n");
  fprintf(fp, "%d %d 255\n", w, h);
  fwrite(pixels, 3, w * h, fp);
  fclose(fp);
}

