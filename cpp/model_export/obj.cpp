#include "obj.h"

#include <sstream>
#include <cstring>
#include <set>
#include <cstdarg>
#include <sstream>

namespace wavefront {

void parse(Vector3 &v, const char *text) {
  char temp[1024];
  strcpy(temp, text);
  v.x = (float) strtod(strtok(temp, " \n"), NULL);
  v.y = (float) strtod(strtok( NULL, " \n"), NULL);
  v.z = (float) strtod(strtok( NULL, " \n"), NULL);
}

void parse(Vector2 &v, const char *text) {
  char temp[1024];
  strcpy(temp, text);
  v.x = (float) strtod(strtok(temp, " \n"), NULL);
  v.y = (float) strtod(strtok( NULL, " \n"), NULL);
}

bool Vertex::operator <(const Vertex &rhs) const {
  if (vp < rhs.vp)
    return true;
  else if (vp > rhs.vp)
    return false;
  else {
    if (vn < rhs.vn)
      return true;
    else if (vn > rhs.vn)
      return false;
    else {
      if (vt < rhs.vt)
        return true;
      else if (vt >= rhs.vt)
        return false;
    }
  }
  return false;
}

bool Vertex::operator >(const Vertex &rhs) const {
  return rhs < *this;
}

bool Vertex::operator ==(const Vertex &rhs) const {
  return vp == rhs.vp && vn == rhs.vn && vt == rhs.vt;
}

void Vertex::parse(const char *text) {
  char temp[1024];
  strcpy(temp, text);

  vn = vt = -1;

  int32 numSlashes = 0;
  for (int32 i = 0; temp[i] != '\0'; i++)
    if ('/' == temp[i])
      numSlashes++;

  if (0 == numSlashes) {
    vp = atoi(temp) - 1;
    return;
  }

  if (1 == numSlashes) {
    vp = atoi(strtok(temp, "/")) - 1;
    vn = atoi(strtok( NULL, " ")) - 1;
    return;
  }

  if (2 == numSlashes) {
    int32 i = 0;
    char *toks[2];
    for (i = 0; temp[i] != '\0'; i++)
      if ('/' == temp[i]) {
        temp[i] = '\0';
        toks[0] = &temp[i + 1];
        break;
      }

    i++;
    for (; temp[i] != '\0'; i++)
      if ('/' == temp[i]) {
        temp[i] = '\0';
        toks[1] = &temp[i + 1];
        break;
      }

    vp = atoi(temp) - 1;

    if (*toks[0] != '\0')  //int the case of 'vp//vn', where there is no 'vt'
      vt = atoi(toks[0]) - 1;

    vn = atoi(toks[1]) - 1;
  }
}

bool SimpleObj::loadFromFile(std::string_view dir, std::string_view name, int frame) {
  std::stringstream ss;
  ss << dir;
  ss << "/";
  ss << name;
  if (frame > 0) {
    char buff[64];
    sprintf(buff, "_%.6u", frame);
    ss << std::string(buff);
  }
  ss << ".obj";
  filename = ss.str();

  FILE *fp = fopen(filename.c_str(), "r");
  if (!fp) {
    printf("%s - file '%s' not found\n", __FUNCTION__, filename.c_str());
    filename = "";
    return false;
  }
  printf("%s - loading OBJ data from file '%s'\n", __FUNCTION__, filename.c_str());
  coords.clear();
  nos.clear();
  uvs.clear();
  faces.clear();

  char line[256];
  while (fgets(line, sizeof(line), fp)) {

    if ('v' == line[0] && ' ' == line[1]) {
      Vector3 v;
      parse(v, &line[2]);
      coords.push_back(v);
    }

    if ('v' == line[0] && 'n' == line[1]) {
      Vector3 v;
      parse(v, &line[3]);  //skip 3 chars
      nos.push_back(v);
    }

    if ('v' == line[0] && 't' == line[1]) {
      Vector2 v;
      parse(v, &line[3]);  //skip 3 chars
      uvs.push_back(v);
    }

    if ('f' == line[0] && ' ' == line[1]) {
      char *toks[3];
      toks[0] = strtok(&line[2], " \n");
      toks[1] = strtok( NULL, " \n");
      toks[2] = strtok( NULL, " \n");

      Face f;
      f.v0.parse(toks[0]);
      f.v1.parse(toks[1]);
      f.v2.parse(toks[2]);
      faces.push_back(f);
    }
  }
  printf(" * no. of vertices.: %zu\n", coords.size());
  printf(" * no. of normals..: %zu\n", nos.size());
  printf(" * no. of texcoords: %zu\n", uvs.size());
  printf(" * no. of faces....: %zu\n", faces.size());
  return true;
}

bool ColoredObj::Vertex::operator <(const Vertex &rhs) const {
  if (p < rhs.p)
    return true;
  if (p > rhs.p)
    return false;

  if (n < rhs.n)
    return true;

  if (t < rhs.t)
    return true;
  if (t > rhs.t)
    return false;

  return false;
}

bool ColoredObj::Vertex::operator >(const Vertex &rhs) const {
  return rhs < *this;
}

bool ColoredObj::Vertex::operator ==(const Vertex &rhs) const {
  return p == rhs.p && t == rhs.t && t == rhs.t;
}

bool ColoredObj::load(std::string_view fileName) {
  const char *tag = "ColoredObj::Load";

  //0,1,2, info/warn/error
  auto print = [&](int type, std::string_view format, ...) {
    std::stringstream ss;
    char buffer[1024];
    if (0 == type)
      ss << "[INFO]..: " << tag << " - ";
    if (1 == type)
      ss << "[WARN]..: " << tag << " - ";
    if (2 == type)
      ss << "[ERROR].: " << tag << " - ";

    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format.data(), args);
    va_end(args);
    ss << buffer;
    printf("%s\n", ss.str().c_str());
  };

  FILE *fp = fopen(fileName.data(), "r");

  if (!fp) {
    print(2, "file '%s' not found", fileName.data());
    return false;
  }
  print(0, "loading OBJ from file '%s'", fileName.data());

  coords.clear();
  uvs.clear();
  matNames.clear();
  faces.clear();

  char line[256];

  std::string matCurr;
  int matIndex;
  while (fgets(line, sizeof(line), fp)) {
    if ('v' == line[0] && ' ' == line[1]) {
      Vector3 v;
      sscanf(&line[2], "%f, %f, %f", &v.x, &v.y, &v.z);
      coords.push_back(v);
    }

    if ('v' == line[0] && 'n' == line[1]) {
      Vector3 v;
      sscanf(&line[3], "%f, %f, %f", &v.x, &v.y, &v.z);
      nos.push_back(v);
    }

    if ('v' == line[0] && 't' == line[1]) {
      Vector2 v;
      sscanf(&line[3], "%f, %f", &v.x, &v.y);
      uvs.push_back(v);
    }

    if (0 == memcmp("usemtl", line, 6)) {
      matNames.push_back(std::string(strtok(&line[7], " \n")));
      matCurr = matNames.back();
      matIndex = (int) matNames.size() - 1;
    }
    if ('f' == line[0] && ' ' == line[1]) {
      Face face;
      face.m = matIndex;
      sscanf(strtok(&line[2], " \n"), "%d/%d/%d", &face.v.p, &face.v.n, &face.v.uv);
      sscanf(strtok(nullptr, " \n"), "%d/%d/%d", &face.v2.p, &face.v2.n, &face.v2.uv);
      sscanf(strtok(nullptr, " \n"), "%d/%d/%d", &face.v3.p, &face.v3.n, &face.v3.uv);
      faces.push_back(face);
      matFaces[matCurr].push_back((int) faces.size() - 1);
    }
  }
  fclose(fp);

  std::set<Vertex> sortedVerts;
  for (auto &f : faces) {
    for (int i = 0; i < 3; i++) {
      f.verts[i].p = coords[f.vs[i].p - 1];
      f.verts[i].n = nos[f.vs[i].n - 1];
      f.verts[i].t = uvs[f.vs[i].uv - 1];
      sortedVerts.insert(f.verts[i]);
    }
  }

  // these are now sorted
  for (auto vert : sortedVerts)
    vertices.push_back(vert);

  auto find = [&](const Vertex &v) {
    int l = 0;
    int r = (int) vertices.size() - 1;
    while (l <= r) {
      int m = (l + r) / 2;
      if (vertices[m] < v)
        l = m + 1;
      else if (vertices[m] > v)
        r = m - 1;
      return m;
    }
    return -1;
  };

  for (auto &face : faces) {
    face.ids[0] = find(face.verts[0]);
    face.ids[1] = find(face.verts[1]);
    face.ids[2] = find(face.verts[2]);
    indices.push_back(face.ids[0]);
    indices.push_back(face.ids[1]);
    indices.push_back(face.ids[2]);
  }

  return true;
}

bool ColoredObj::loadMaterials(std::string_view fileName) {
  const char *tag = "ColoredObj::laodMaterials";

  //0,1,2, info/warn/error
  auto print = [&](int type, std::string_view format, ...) {
    std::stringstream ss;
    char buffer[1024];
    if (0 == type)
      ss << "[INFO]..: " << tag << " - ";
    if (1 == type)
      ss << "[WARN]..: " << tag << " - ";
    if (2 == type)
      ss << "[ERROR].: " << tag << " - ";

    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format.data(), args);
    va_end(args);
    ss << buffer;
    printf("%s\n", ss.str().c_str());
  };

  FILE *fp = fopen(fileName.data(), "r");

  if (!fp) {
    print(2, "file '%s' not found", fileName.data());
    return false;
  }
  print(0, "loading OBJ materials from file '%s'", fileName.data());

  char line[256];
  int lineNo = -1;
  while (fgets(line, sizeof(line), fp)) {
    lineNo++;
    if ('#' == line[0])
      continue;
    auto *p = strchr(line, '\n');
    if (p)
      *p = '\0';

    print(0, "%s", line);

    char *toks[3];
    toks[0] = strtok(line, " \n");
    toks[1] = strtok(nullptr, " \n");
    toks[2] = strtok(nullptr, " \n");
    if (toks[0] == nullptr || toks[1] == nullptr || toks[2] == nullptr) {
      print(1, "invalid material at line %d", lineNo);
    }
    auto &material = materials[std::string(toks[0])];
    material.name = std::string(toks[0]);
    material.albedo.fromHex(toks[1]);
    material.emission.fromHex(toks[2]);
  }

  return true;
}

}
