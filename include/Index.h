#ifndef INDEX_H
#define INDEX_H
#include "Utils.h"
#include <string>
#include <map>
#include <vector>
#include <set>
using std::vector;
using std::string;
using std::map;
using std::set;
class Index { // 这个 index 是为了 staging area 实现的接口
public:
    std::map<string, string> added; // filename -> blob_hash
    std::set<string> removed;       // filename

    void readFromDisk(); // 从 .gitlite/index 读取
    void writeToDisk();  // 写入 index
    void add(string filename, string blobHash);
    void remove(string filename);
};
#endif