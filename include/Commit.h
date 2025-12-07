#ifndef COMMIT_H
#define COMMIT_H
#include <string>
#include <vector>
#include <string>
#include <ctime>
#include <map>
using std::string;
class Commit{
   private:
      string father_hash;
      string second_parent_hash; // 默认为空，只有 merge 时有值
      string Hash; // 自己的 hash，方便文件命名
      string author; 
      string message;
      std::time_t time_stamp; // initial commit 的 time stamp 为 0
      std::map<string, string> file_blob_map; // file hash & blob hash，表示当前跟踪文件，下次commit的时候会先复制这里
   public:
      // basic functions
      Commit() = default;
      Commit(string father, string auth, string mess); // 这个函数实现 git commit，内部调用deserial
      ~Commit() = default;

      string get_hash();

      // Commit 的序列化与反序列化
      static std::vector<unsigned char> commit_serial(const Commit& cmt);
      static Commit commit_deserial(const std::string Hs); // 这里不能随意返回引用，不然会造成局部变量引用的问题
};
#endif // COMMIT_H
