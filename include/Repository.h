#ifndef REPOSITORY_H
#define REPOSITORY_H
#include "Utils.h"
#include <string>
#include <unistd.h>
#include "Commit.h"
#include "Blob.h"
#include <ctime>
#include "Index.h"
using std::string;

class Repository{
   private:
      // Directory: index: 
      //    file: stage_add 代表 add 的文件名和 blob 的关系
      //    file: stage_remove 代表待删除的文件名，由 rm 造成
      // Directory: branches: 代表不同的 branch 所对应的 commit 记录（以 hash 表示）
      // Directory: blobs: add 后出现，add 一并更新staging area，此处保存 blobs
      // Directory: commits：分散的 commit，文件名为其 hash
      // file：head 直接记录当前 branch（默认）
      string CUR_DIR;
      string GIT_DIR;
      string INDEX_DIR;
      string BLOBS_DIR;
      string COMMITS_DIR;
      string BRANCHES_DIR;
      string HEAD_PATH;
   public:
      // 基本函数
      Repository(); // 这个是需要实现的，因为除了init之外新建的Repo也不认识这些DIR 
      ~Repository() = default;

      // git函数
      // 注：这里记得各个 string 都要是 const& 格式，否则无法传入字面量
      // Subtask 1
      void init();
      void add(const string& filename);
      void commit(const string& message);
      void rm(const string& filename);

      // // Subtask 2
      void log();
      void globalLog(); 
      void find(const string& target_message); 

      // // Subtask 3 (basic checkout is categorized here for integrity)
      void status();
      void checkoutBranch(const string& branchname);
      void checkoutFile(const string& filename);
      void checkoutFileInCommit(const string& abbr_commit, const string& filename);

      // // Subtask 4
      // void branch(const string& branchname);
      // void rmBranch(const string& branchname);
      // void reset(const string commitid);

      // // Subtask 5
      // void merge(const string& branchname);

      // // Subtask 6 (Bonus) will I finish these functions? who knows!
      // void addRemote(const string& remotename, const string& remote_dir);
      // void rmRemote(const string& remotename);
      // void push(const string& remotename, const string& rm_branch);
      // void fetch(const string& remotename, const string& rm_branch);
      // void pull(const string& remotename, const string& rm_branch);

      // 静态函数 & helpers
      static std::string getWorkingDir();
      static std::string getGitliteDir();
      static std::string getCommitsDir();
      static std::string getBranchesDir();
      static std::string getIndexDir();
      static std::string getBlobsDir();
      static std::string getHeadsPath();
      static string getHeadbranch();
      static string getHeadhash();
      static Commit getHeadCommit();
      static void rewriteHead(const string& branchname);
      static string getBranchhash(const string& branchname);
      static Commit getBranchCommit(const string & branchname);
};
#endif // REPOSITORY_H
