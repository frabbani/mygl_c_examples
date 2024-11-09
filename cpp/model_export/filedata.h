#pragma once

#include <cstdint>
#include <string>
#include <vector>

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
    fseek(fp, 0, SEEK_SET);
    name = fileName;
    data = std::vector<uint8_t>(size);
    fread(data.data(), size, 1, fp);
    fclose(fp);
  }

  static bool exists(std::string_view fileName) {
    FILE *fp = fopen(fileName.data(), "r");
    if (fp) {
      fclose(fp);
      return true;
    }
    return false;
  }
};
