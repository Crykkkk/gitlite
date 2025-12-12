#include "../include/Repository.h"
#include <algorithm>
using std::string;
using std::cout;
using std::endl;

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
   Commit Initial_cm("", "initial commit"); // 这一步读取master然后更新branches，commits，index
   Initial_cm.save_commit();
}

void Repository::add(const string& filename){
   // 在工作目录下找到文件并变成blob（注：blob已经实现了报错）
   Blob target_file = Blob(filename);
   Index index;
   index.readFromDisk();
   // 下面进行 add 逻辑
   if (index.removed.count(filename)) index.removed.erase(filename);

   Commit curr_cm = getHeadCommit();
   if (curr_cm.check_map().count(filename) && curr_cm.check_map()[filename] == target_file.Hash) {
      if (index.added.count(filename)) index.added.erase(filename);
      index.writeToDisk();
      return;
   }

   index.added[filename] = target_file.Hash;
   target_file.save_blob();
   
   index.writeToDisk();
   return;
}

void Repository::commit(const string& message) {
   if (message == "") {
      Utils::exitWithMessage("Please enter a commit message.");
   }
   Index index;
   index.readFromDisk();
   // 更新 file blob map
   if (index.added.empty() && index.removed.empty()) {
      Utils::exitWithMessage("No changes added to the commit.");
   }

   Commit my_cm = Commit(getHeadhash(), message);
   for (auto i : index.added) {
      my_cm.check_map()[i.first] = i.second;
   }
   for (auto i : index.removed) {
      my_cm.check_map().erase(i);
   }
   
   // save
   my_cm.save_commit();

   // 清空暂存区
   index.added.clear();
   index.removed.clear();
   index.writeToDisk();
}

void Repository::rm(const string& filename) {
   Commit curr_cm = getHeadCommit();
   Index index;
   index.readFromDisk();
   if (!curr_cm.check_map().count(filename) && !index.added.count(filename)) {
      Utils::exitWithMessage("No reason to remove the file.");
   }
   if (index.added.count(filename)) {
      index.added.erase(filename);
   }
   if (curr_cm.check_map().count(filename)) {
      index.removed.insert(filename);
      string rm_dir = Utils::join(getWorkingDir(), filename);
      Utils::restrictedDelete(rm_dir);
   }
   index.writeToDisk(); // 怎么老是忘记这行。。
}

void Repository::log() {
   Commit curr_cm = getHeadCommit();
   while (true) {
      curr_cm.show();
      cout << endl;
      if (curr_cm.father_hash != "") {
         curr_cm = Commit::commit_deserial(curr_cm.father_hash);
      }
      else break;
   }
}

void Repository::globalLog() {
   auto ite_cm = Utils::plainFilenamesIn(getCommitsDir());
   for (string& cm : ite_cm) {
      Commit curr_cm = Commit::commit_deserial(cm);
      curr_cm.show();
      cout << endl;
   }
}

void Repository::find(const string& target_message) {
   int flag = 0;
   auto ite_cm = Utils::plainFilenamesIn(getCommitsDir());
   for (string& cm : ite_cm) {
      Commit curr_cm = Commit::commit_deserial(cm);
      if (curr_cm.message == target_message) {
         flag = 1;
         cout << curr_cm.Hash << endl;
      }
   }
   if (!flag) Utils::exitWithMessage("Found no commit with that message.");
}

void Repository::status() {
   cout << "=== Branches ===" << endl;
   auto ite_branch = Utils::plainFilenamesIn(getBranchesDir());
   std::sort(ite_branch.begin(), ite_branch.end());
   for (string& bc : ite_branch) {
      if (bc != getHeadbranch()) {
         cout << bc << endl;
      }
      else {
         cout << "*" << bc << endl;
      }
   }
   cout << endl;
   
   Index index;
   index.readFromDisk();
   cout << "=== Staged Files ===" << endl;
   for (auto ad : index.added) {
      cout << ad.first << endl;
   }
   cout << endl;

   cout << "=== Removed Files ===" << endl;
   for (auto rm : index.removed) {
      cout << rm << endl;
   }
   cout << endl;

   cout << "=== Modifications Not Staged For Commit ===" << endl;
   // TODO
   cout << endl;

   cout << "=== Untracked Files ===" << endl;
   // TODO
   cout << endl;
}

void Repository::checkoutFile(const string& filename) {
   checkoutFileInCommit(getHeadhash(), filename);
}

void Repository::checkoutBranch(const string& branchname) {
   auto ite_branch = Utils::plainFilenamesIn(getBranchesDir());
   if (std::find(ite_branch.begin(), ite_branch.end(), branchname) == ite_branch.end()) {
      Utils::exitWithMessage("No such branch exists.");
   }
   if (getHeadbranch() == branchname) {
      Utils::exitWithMessage("No need to checkout the current branch.");
   }
   
   Commit Terminater_cm = getBranchCommit(branchname); // 终结者！
   Commit Curr_cm = getHeadCommit();

   auto ite_files = Utils::plainFilenamesIn(getWorkingDir());
   for (string& file : ite_files) {
      if (!Curr_cm.check_map().count(file) && Terminater_cm.check_map().count(file)) {
         Utils::exitWithMessage("There is an untracked file in the way; delete it, or add and commit it first.");
      }
   }
   // readme讲的有点难绷这里的逻辑

   // 真正开始处理
   Index index = Index();
   index.writeToDisk(); // 清空暂存区
   
   rewriteHead(branchname);

   for (string& file : ite_files) {
      if (Curr_cm.check_map().count(file) && !Terminater_cm.check_map().count(file)) {
         Utils::restrictedDelete(Utils::join(getWorkingDir(), file));
      }
   }
   for (auto file_pair : Terminater_cm.check_map()) {
      string target_blob = file_pair.second;
      string target_content = Blob::blob_deserial_content(target_blob);
      string rewrite_path = Utils::join(getWorkingDir(), file_pair.first);
      Utils::writeContents(rewrite_path, target_content);
   }
}

void Repository::checkoutFileInCommit(const string& abbr_commit, const string& filename) {
   // 获取 curr_cm
   Commit curr_cm;
   if (abbr_commit.length() == 40) {
      if (Utils::isFile(Utils::join(getCommitsDir(), abbr_commit))) {
         curr_cm = Commit::commit_deserial(abbr_commit);
      }
      else Utils::exitWithMessage("No commit with that id exists.");
   }
   else {
      int lth = abbr_commit.length();
      int flag = 0;
      auto ite_cm = Utils::plainFilenamesIn(getCommitsDir());
      for (string& cm_hs : ite_cm) {
         if (cm_hs.substr(0, lth) == abbr_commit) {
            curr_cm = Commit::commit_deserial(cm_hs);
            flag = 1;
            break;
         }
      }
      if (!flag) Utils::exitWithMessage("No commit with that id exists.");
   }

   if (curr_cm.check_map().count(filename)) {
      string target_blob = curr_cm.check_map()[filename];
      string target_content = Blob::blob_deserial_content(target_blob);
      string rewrite_path = Utils::join(getWorkingDir(), filename);
      Utils::writeContents(rewrite_path, target_content);
      return;
   }
   Utils::exitWithMessage("File does not exist in that commit.");
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
string Repository::getHeadhash() { // 得到 commit hash
   string target_branch = getHeadbranch();
   string target_hash = Utils::join(getBranchesDir(), target_branch);
   return Utils::readContentsAsString(target_hash);
}
Commit Repository::getHeadCommit() {
   return Commit::commit_deserial(getHeadhash());
}
string Repository::getBranchhash(const string& branchname) {
   string target_hash = Utils::join(getBranchesDir(), branchname);
   return Utils::readContentsAsString(target_hash);
}
Commit Repository::getBranchCommit(const string & branchname) {
   return Commit::commit_deserial(getBranchhash(branchname));
}

void Repository::rewriteHead(const string& branchname) {
   string head_path = getHeadsPath();
   Utils::writeContents(head_path, branchname);
}