#include "../include/Repository.h"
using std::string;

// the implementation of the Repository class
void Repository::init() {
   char cwd[2048];
   CUR_DIR = getcwd(cwd, sizeof(cwd));
   // 在CUR_DIR下创建 .gitlite
   // 判断是否已经存在
   // Directory: index: 
   //    file: stage_add 代表 add 的文件名和 blob 的关系
   //    file: stage_remove 代表待删除的文件名，由 rm 造成
   // Directory: branches: 代表不同的 branch 所对应的 commit 记录（以 hash 表示）
   // Directory: blobs: add 后出现，add 一并更新staging area，此处保存 blobs
   // Directory: commits：分散的 commit，文件名为其 hash

   // 进行 initial commit
}
// To be continued......

// the static functions of the Repository
string Repository::getGitliteDir() {
   char cwd[2048];
   string BASE_DIR = getcwd(cwd, sizeof(cwd));
   string GitlitDir = Utils::join(BASE_DIR, ".gitlite");
   return GitlitDir;
}