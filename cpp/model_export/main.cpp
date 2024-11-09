#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#define _USE_MATH_DEFINES
#include <cmath>
#include <string>
#include <sstream>
#include <vector>
#include <set>

#include "obj.h"
#include "filedata.h"

using namespace wavefront;


const std::string_view name = "doomguy";
const std::string_view loadDir = "assets/doomguy";
const std::string_view exportDir = "export/doomguy";

// used by both export routines
std::vector<Vertex> verts;

std::string objFrameFileName(size_t frameNo) {
  char token[256];
  sprintf(token, "%s/%s_%.6zu.obj", loadDir.data(), name.data(), frameNo);
  return token;
}

void exportBaseFromOBJ(const SimpleObj &obj) {
  struct Tri {
    int i, j, k;
  };

  std::vector<Tri> tris;

  std::set<Vertex> vertSet;
  for (auto face : obj.faces) {
    vertSet.insert(face.v0);
    vertSet.insert(face.v1);
    vertSet.insert(face.v2);
  }

  verts.clear();
  for (auto v : vertSet)
    verts.push_back(v);

  auto getIndex = [&](Vertex v) -> int {
    size_t l = 0;
    size_t r = verts.size() - 1;
    while (l <= r) {
      size_t m = (l + r) / 2;
      if (verts[m] < v)
        l = m + 1;
      else if (verts[m] > v)
        r = m - 1;
      else
        return (int) m;
    }
    return -1;
  };

  tris.clear();
  for (auto face : obj.faces) {
    Tri tri;
    tri.i = getIndex(face.v0);
    tri.j = getIndex(face.v1);
    tri.k = getIndex(face.v2);
    tris.push_back(tri);
  }

  printf("%s results:\n", __FUNCTION__);
  printf(" * no. of vertices.: %zu\n", verts.size());
  printf(" * no. of triangles: %zu\n", tris.size());

  std::stringstream ss;
  ss << exportDir << "/mesh.txt";

  FILE *fp = fopen(ss.str().c_str(), "w");
  for (auto v : verts) {
    fprintf(fp, "v ");
    fprintf(fp, "%f,%f,%f %f,%f,%f %f,%f\n", obj.coords[v.vp].x,
            obj.coords[v.vp].y, obj.coords[v.vp].z, obj.nos[v.vn].x,
            obj.nos[v.vn].y, obj.nos[v.vn].z, obj.uvs[v.vt].x, obj.uvs[v.vt].y);
  }
  for (auto t : tris) {
    fprintf(fp, "f %d,%d,%d\n", t.i, t.j, t.k);
  }
  fclose(fp);

}

void exportFrameFromObj(const SimpleObj &obj, int frameNo) {
  if (frameNo <= 0)
    return;

  std::stringstream ss;
  ss << exportDir;
  ss << "/";
  ss << "frame_";
  ss << frameNo;
  ss << ".txt";

  FILE *fp = fopen(ss.str().c_str(), "w");
  for (auto v : verts) {
    fprintf(fp, "v %f,%f,%f %f,%f,%f\n", obj.coords[v.vp].x, obj.coords[v.vp].y,
            obj.coords[v.vp].z, obj.nos[v.vn].x, obj.nos[v.vn].y,
            obj.nos[v.vn].z);
  }
  fclose(fp);
  printf("frame '%s' created\n", ss.str().c_str());
}

int main() {
  printf("hello world!\n");
  SimpleObj obj;
  obj.loadFromFile(loadDir, name, 0);

  exportBaseFromOBJ(obj);
  for (size_t i = 1; i < 256; i++) {
    if (!FileData::exists(objFrameFileName(i)))
      break;
    obj.loadFromFile(loadDir, name, i);
    exportFrameFromObj(obj, i);
  }

  printf("goodbye!\n");
  return 0;
}
