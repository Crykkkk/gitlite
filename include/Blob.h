#ifndef BLOB_H
#define BLOB_H
#include <string>
#include <vector>
#include "Utils.h"
using std::string;
using std::vector;
class Blob{
   private:
      string target_file;
      vector<unsigned char> content; // 快照机制下需要在创建的时候就存储好
      string Hash;
   public:
      Blob(string& filename);
      void save_blob(); // 存储到对应位置
};
#endif