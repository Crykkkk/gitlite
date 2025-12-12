#ifndef BLOB_H
#define BLOB_H
#include <string>
#include <vector>
#include "Utils.h"
using std::string;
using std::vector;
class Blob{
   public:
      string target_file;
      vector<unsigned char> content; // 快照机制下需要在创建的时候就存储好
      string Hash;

      // 成员函数
      Blob(const string& filename);
      Blob() = default;
      void save_blob(); // 存储到对应位置

      // 静态函数（为了避免和 filename 构造函数混淆，采用这种方式实现反序列化）
      static string blob_deserial_content(const string& blobhash);
};
#endif