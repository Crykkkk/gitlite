#include "../include/Repository.h"
#include <algorithm>
#include <filesystem>
#include <sstream>
#include <set>
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
   REMOTE_DIR = Utils::join(GIT_DIR, "remote");
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

void Repository::merge_commit(const string& message, const string& extra_father) {
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
   my_cm.second_parent_hash = extra_father; // 加了这行

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

   std::set<string> not_staged;
   std::set<string> untrack;
   Commit curr_cm = getHeadCommit();
   std::map<string, string> work_hash;
   for (auto file : Utils::plainFilenamesIn(getWorkingDir())) {
      work_hash[file] = Blob(file).Hash;
      if (!curr_cm.check_map().count(file) && !index.added.count(file)) {
         untrack.insert(file);
      }
      if (index.removed.count(file)) {
         untrack.insert(file);
      }
   }
   for (auto file_pair : curr_cm.check_map()) {
      if (!work_hash.count(file_pair.first) && !index.removed.count(file_pair.first)) {
         not_staged.insert(file_pair.first + " (deleted)");
      }
      else if (work_hash.count(file_pair.first) && (work_hash[file_pair.first] != file_pair.second) && !index.added.count(file_pair.first)) {
         not_staged.insert(file_pair.first + " (modified)");
      } 
   }
   for (auto file_pair : index.added) {
      if (!work_hash.count(file_pair.first)) {
         not_staged.insert(file_pair.first + " (deleted)");
      }
      else if (work_hash[file_pair.first] != file_pair.second) {
         not_staged.insert(file_pair.first + " (modified)");
      }
   }


   cout << "=== Modifications Not Staged For Commit ===" << endl;
   for (auto file : not_staged) {
      cout << file << endl;
   }
   cout << endl;

   cout << "=== Untracked Files ===" << endl;
   for (auto file : untrack) {
      cout << file << endl;
   }
   cout << endl;
}

void Repository::checkoutFile(const string& filename) {
   checkoutFileInCommit(getHeadhash(), filename);
}

void Repository::checkoutBranch(const string& branchname) {
   string branch_path = Utils::join(getBranchesDir(), branchname);
   if (!Utils::isFile(branch_path)) {
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

void Repository::branch(const string& branchname) {
   string target_path = Utils::join(getBranchesDir(), branchname);
   if (Utils::isFile(target_path)) {
      Utils::exitWithMessage("A branch with that name already exists.");
   }
   Utils::writeContents(target_path, getHeadhash());
}

void Repository::rmBranch(const string& branchname) {
   string target_path = Utils::join(getBranchesDir(), branchname);
   if (!Utils::isFile(target_path)) {
      Utils::exitWithMessage("A branch with that name does not exist.");
   }
   if (branchname == getHeadbranch()) {
      Utils::exitWithMessage("Cannot remove the current branch.");
   }
   remove(target_path.c_str());
}

void Repository::reset(const string commitid) {
   // 检查
   string check_cm_path = Utils::join(getCommitsDir(), commitid);
   if (!Utils::isFile(check_cm_path)) {
      Utils::exitWithMessage("No commit with that id exists.");
   }

   // 准备
   string cover_branch = "__gitlite_temp_reset_branch__"; // 工具分支
   string last_head = getHeadbranch();
   string cover_path = Utils::join(getBranchesDir(), cover_branch);
   string last_head_path = Utils::join(getBranchesDir(), last_head);

   // 修改
   Utils::writeContents(cover_path, commitid);
   checkoutBranch(cover_branch);
   
   // 夺舍
   rewriteHead(last_head);
   Utils::writeContents(last_head_path, commitid);
   rmBranch(cover_branch);
}

void Repository::merge(const string& branchname) {
   // 检查
   if (branchname == getHeadbranch()) {
      Utils::exitWithMessage("Cannot merge a branch with itself.");
   }
   if (!Utils::isFile(Utils::join(BRANCHES_DIR, branchname))) {
      Utils::exitWithMessage("A branch with that name does not exist.");
   }
   Index index;
   index.readFromDisk();
   if (!index.added.empty() || !index.removed.empty()) {
      Utils::exitWithMessage("You have uncommitted changes.");
   }

   // Calculate LCA
   string LCA_hs = Commit::lowest_common_ancestor(getHeadhash(), getBranchhash(branchname));
   if (LCA_hs == getBranchhash(branchname)) {
      Utils::exitWithMessage("Given branch is an ancestor of the current branch.");
   }
   if (LCA_hs == getHeadhash()) {
      checkoutBranch(branchname);
      Utils::exitWithMessage("Current branch fast-forwarded.");
   }

   // 正经情况 TODO
   Commit LCA_cm = Commit::commit_deserial(LCA_hs);
   Commit Curr_cm = getHeadCommit();
   Commit other_cm = getBranchCommit(branchname);

   // 总文件去重
   std::set<string> Allfile;
   for (auto file_pair : LCA_cm.check_map()) {
      Allfile.insert(file_pair.first);
   }
   for (auto file_pair : Curr_cm.check_map()) {
      Allfile.insert(file_pair.first);
   }
   for (auto file_pair : other_cm.check_map()) {
      Allfile.insert(file_pair.first);
   }

   // real work!
   bool conflict = false;

   for (string file : Allfile) {
      string lca_hash = LCA_cm.check_map().count(file) ? LCA_cm.check_map()[file] : "";
      string curr_hash = Curr_cm.check_map().count(file) ? Curr_cm.check_map()[file] : "";
      string other_hash = other_cm.check_map().count(file) ? other_cm.check_map()[file] : "";

      if (lca_hash == curr_hash && lca_hash != other_hash && other_hash != "") {
         if (curr_hash == "" && Utils::isFile(Utils::join(getWorkingDir(), file))) {
             Utils::exitWithMessage("There is an untracked file in the way; delete it, or add and commit it first.");
         }
      }
   }

   for (string file : Allfile) {
      string lca_hash = LCA_cm.check_map().count(file) ? LCA_cm.check_map()[file] : "";
      string curr_hash = Curr_cm.check_map().count(file) ? Curr_cm.check_map()[file] : "";
      string other_hash = other_cm.check_map().count(file) ? other_cm.check_map()[file] : "";
      if (curr_hash == other_hash) continue;
      else if (lca_hash == curr_hash) {
         if (other_hash == "") {
            this->rm(file);
         }
         else {
            if (curr_hash == "" && Utils::isFile(Utils::join(getWorkingDir(), file))) {
               Utils::exitWithMessage("There is an untracked file in the way; delete it, or add and commit it first.");
            }
            checkoutFileInCommit(other_cm.Hash, file);
            this->add(file);
         }
      }
      else if (lca_hash == other_hash) {
         continue;
      }
      else { // lca_hash != other_hash != curr_hash : conflict
         conflict = true;
         std::ostringstream oss;
         string curr_cnt = ((curr_hash == "") ? "" : Blob::blob_deserial_content(curr_hash));
         string other_cnt = ((other_hash == "") ? "" : Blob::blob_deserial_content(other_hash));
         oss << "<<<<<<< HEAD" << endl;
         oss << curr_cnt;
         if (curr_cnt != "" && curr_cnt.back() != '\n') oss << endl;
         oss << "=======" << endl;
         oss << other_cnt;
         if (other_cnt != "" && other_cnt.back() != '\n') oss << endl;
         oss << ">>>>>>>" << endl;
         string target_file = Utils::join(getWorkingDir(), file);
         Utils::writeContents(target_file, oss.str());
         this->add(file);
      }
   }
   
   if (conflict) std::cout << "Encountered a merge conflict." << std::endl;

   std::ostringstream oss;
   oss << "Merged " << branchname << " into " << getHeadbranch() << '.';
   this->merge_commit(oss.str(), other_cm.Hash);
}

void Repository::addRemote(const string& remotename, const string& remote_dir) {
   string target_remote = Utils::join(getRemotePath(), remotename);
   if (Utils::isFile(target_remote)) {
      Utils::exitWithMessage("A remote with that name already exists.");
   }
   else {
      std::filesystem::path p(remote_dir);
      Utils::writeContents(target_remote, p.make_preferred().string());
   }
}
void Repository::rmRemote(const string& remotename) {
   string target_remote = Utils::join(getRemotePath(), remotename);
   if (!Utils::isFile(target_remote)) {
      Utils::exitWithMessage("A remote with that name does not exist.");
   }
   else {
      remove(target_remote.c_str());
   }
}

std::string cleanString(std::string s) { // 似乎是需要的
    s.erase(0, s.find_first_not_of(" \n\r\t"));
    s.erase(s.find_last_not_of(" \n\r\t") + 1);
    return s;
}

void Repository::push(const string& remotename, const string& rm_branch) {
   string target_remote = Utils::join(getRemotePath(), remotename);
   string remote_root = cleanString(Utils::readContentsAsString(target_remote));

   if (!Utils::isDirectory(remote_root)) Utils::exitWithMessage("Remote directory not found.");

   string rtarget_branch = Utils::join(getBranchesDir(remote_root), rm_branch);

   std::vector<string> target_history; // 记录 target_branch 对应的 commits，从新到老（暂时不考虑two father，懒得搞）
   string curr_hash = getBranchhash(rm_branch);
   
   // 遍历本地历史
   while (curr_hash != "") {
      target_history.push_back(curr_hash);
      Commit cm = Commit::commit_deserial(curr_hash);
      curr_hash = cm.father_hash; 
   }
   
   if (Utils::isFile(rtarget_branch)) {
      string remote_head = cleanString(Utils::readContentsAsString(rtarget_branch));
      if (std::find(target_history.begin(), target_history.end(), remote_head) == target_history.end()) {
         Utils::exitWithMessage("Please pull down remote changes before pushing.");
      }
   }

   for (const string& Hash : target_history) {
      copyObject(Hash, getCommitsDir(), getCommitsDir(remote_root));
      Commit curr_cm = Commit::commit_deserial(Hash);
      for (auto file_pair : curr_cm.check_map()) {
         copyObject(file_pair.second, getBlobsDir(), getBlobsDir(remote_root));
      }
   }

   Utils::writeContents(rtarget_branch, getBranchhash(rm_branch));
}

void Repository::fetch(const string& remotename, const string& rm_branch) {
   string target_remote = Utils::join(getRemotePath(), remotename);
   string remote_root = cleanString(Utils::readContentsAsString(target_remote));

   string rtarget_branch = Utils::join(getBranchesDir(remote_root), rm_branch);

   if (!Utils::isDirectory(remote_root)) {
      Utils::exitWithMessage("Remote directory not found.");
   }
   if (!Utils::isFile(rtarget_branch)) {
      Utils::exitWithMessage("That remote does not have that branch.");
   }
   
   string new_branch_name = remotename + '/' + rm_branch;
   string new_head = cleanString(Utils::readContentsAsString(rtarget_branch));
   Utils::writeContents(Utils::join(getBranchesDir(), new_branch_name), new_head);

   // Commit rCurr_cm = Commit::commit_deserial(new_head); 这么写是错误的，因为还没copy过来呢就解析了
   string curr_fetch = new_head;

   while (curr_fetch != "") {
      if (Utils::isFile(Utils::join(getCommitsDir(), curr_fetch))) {
          break; 
      }
      copyObject(curr_fetch, getCommitsDir(remote_root), getCommitsDir()); 
      Commit cm = Commit::commit_deserial(curr_fetch);

      for (auto const& pair : cm.check_map()) {
         copyObject(pair.second, getBlobsDir(remote_root), getBlobsDir());
      }
      curr_fetch = cm.father_hash;
   }
}

void Repository::pull(const string& remotename, const string& rm_branch) {
   fetch(remotename, rm_branch);
   string new_branch_name = remotename + '/' + rm_branch;
   merge(new_branch_name);
}



// the static functions of the Repository
string Repository::getWorkingDir() {
   char cwd[2048];
   string BASE_DIR = getcwd(cwd, sizeof(cwd));
   return BASE_DIR;
}
string Repository::getGitliteDir(string root) { // 注意 root 本身就有 gitlite 了
   string GitlitDir = (root == "") ? (Utils::join(getWorkingDir(), ".gitlite")) : root;
   return GitlitDir;
}
string Repository::getCommitsDir(string root) {
   return Utils::join(getGitliteDir(root), "commits");
}
string Repository::getBlobsDir(string root) {
   return Utils::join(getGitliteDir(root), "blobs");
}
string Repository::getBranchesDir(string root) {
   return Utils::join(getGitliteDir(root), "branches");
}
string Repository::getIndexDir(string root) {
   return Utils::join(getGitliteDir(root), "index");
}
string Repository::getHeadsPath(string root) {
   return Utils::join(getGitliteDir(root), "head");
}
string Repository::getRemotePath(string root) {
   return Utils::join(getGitliteDir(root), "remote");
}

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

void Repository::copyObject(string hash, string src, string dest) {
   string src_path = Utils::join(src, hash);
   string dest_path = Utils::join(dest, hash);
   if (Utils::isFile(dest_path)) return;
   auto content = Utils::readContents(src_path);
   Utils::writeContents(dest_path, content);
}