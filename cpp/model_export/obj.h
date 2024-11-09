#pragma once

#include <vector>
#include <string>
#include <string_view>
#include <map>

#include "defs.h"
#include "myvector.h"


namespace wavefront {

struct Vertex {
  int vp, vn, vt;

  Vertex() {
    vp = vn = vt = -1;
  }

  Vertex(int vp, int vn, int vt) {
    this->vp = vp;
    this->vn = vn;
    this->vt = vt;
  }

  Vertex(const char *text) {
    parse(text);
  }

  ~Vertex() {
    vp = vn = vt = -1;
  }

  bool operator <(const Vertex &rhs) const;
  bool operator >(const Vertex &rhs) const;
  bool operator ==(const Vertex &rhs) const;

  void parse(const char *text);

  std::string print() {
    char tmp[256];
    sprintf(tmp, "(%d,%d,%d)", vp, vn, vt);
    return std::string(tmp);
  }
};

struct Face {
  Vertex v0, v1, v2;
};

// Simple, contains no material information
struct SimpleObj {

  std::string filename;
  std::vector<Vector3> coords;
  std::vector<Vector3> nos;
  std::vector<Vector2> uvs;
  std::vector<Face> faces;

  bool loadFromFile(std::string_view dir, std::string_view name, int frame);
};


struct ColoredObj {
  struct Material {
    std::string name;
    Color albedo;
    Color emission;
  };

  struct Vertex {
    Vector3 p;
    Vector3 n;
    Vector2 t;

    bool operator <(const Vertex &rhs) const;
    bool operator >(const Vertex &rhs) const;
    bool operator ==(const Vertex &rhs) const;
  };

  struct Face {
    struct Vertex {
      int p;
      int n;
      int uv;
    };
    union {
      struct {
        Vertex v, v2, v3;
      };
      Vertex vs[3];
    };
    int m;
    ColoredObj::Vertex verts[3];
    int ids[3];
  };

  std::string fileName;
  std::map<std::string, Material> materials;
  std::map<std::string, std::vector<uint16>> matFaces;
  std::vector<Vector3> coords;
  std::vector<Vector3> nos;
  std::vector<Vector2> uvs;
  std::vector<std::string> matNames;
  std::vector<Face> faces;
  std::vector<Vertex> vertices;
  std::vector<uint16> indices;

  bool load(std::string_view fileName);
  bool loadMaterials(std::string_view fileName);
};

}
