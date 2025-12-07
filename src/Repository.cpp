#include "../include/Repository.h"
#include <algorithm>
using std::string;

// the implementation of the Repository class
Repository::Repository(){ // 构造函数，主要是实现文件夹
   char cwd[2048];
   CUR_DIR = getcwd(cwd, sizeof(cwd));
   GIT_DIR = Utils::join(CUR_DIR, ".gitlite");
   INDEX_DIR = Utils::join(GIT_DIR, "index");
   BLOBS_DIR = Utils::join(GIT_DIR, "blobs");
   COMMITS_DIR = Utils::join(GIT_DIR, "commits");
   BRANCHES_DIR = Utils::join(GIT_DIR, "branches");
   HEAD_PATH = Utils::join(GIT_DIR, "head");
}

void Repository::init() {
   // 判断是否已经存在
   if (Utils::isDirectory(GIT_DIR)) {
      Utils::exitWithMessage("A Gitlite version-control system already exists in the current directory.");
   }
   // 开始创建
   Utils::createDirectories(GIT_DIR);
   Utils::createDirectories(INDEX_DIR);
   Utils::createDirectories(BLOBS_DIR);
   Utils::createDirectories(COMMITS_DIR);
   Utils::createDirectories(BRANCHES_DIR);

   // 进行 initial commit
   rewriteHead("master");
   Commit Initial_cm("", "Initial message"); // 这一步读取master然后更新branches，commits，index
   Initial_cm.save_commit();
}

void Repository::add(const string& filename){
   // 在工作目录下找到文件并变成blob（注：blob已经实现了报错）
   Index index;
   index.readFromDisk();
}

// To be continued......





// the static functions of the Repository
string Repository::getWorkingDir() {
   char cwd[2048];
   string BASE_DIR = getcwd(cwd, sizeof(cwd));
   return BASE_DIR;
}
string Repository::getGitliteDir() {
   string GitlitDir = Utils::join(getWorkingDir(), ".gitlite");
   return GitlitDir;
}
string Repository::getCommitsDir() {
   return Utils::join(getGitliteDir(), "commits");
}
string Repository::getBlobsDir() {
   return Utils::join(getGitliteDir(), "blobs");
}
string Repository::getBranchesDir() {
   return Utils::join(getGitliteDir(), "branches");
}
string Repository::getIndexDir() {
   return Utils::join(getGitliteDir(), "index");
}
string Repository::getHeadsPath() {
   return Utils::join(getGitliteDir(), "head");
}
// 以后就直接调用这些函数，千万不要手动输入字符串路径...
string Repository::getHeadbranch() {
   string head_path = getHeadsPath();
   return Utils::readContentsAsString(head_path);
}
string Repository::getHeadhash() {
   string target_branch = getHeadbranch();
   string target_hash = Utils::join(getBranchesDir(), target_branch);
   return Utils::readContentsAsString(target_hash);
}
void Repository::rewriteHead(const string& branchname) {
   string head_path = getHeadsPath();
   Utils::writeContents(head_path, branchname);
}