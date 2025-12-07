#ifndef REPOSITORY_H
#define REPOSITORY_H
#include "Utils.h"
#include <string>
#include <unistd.h>
#include "Commit.h"
#include "Blob.h"
#include <ctime>
using std::string;

class Repository{
   private:
      std::string CUR_DIR;
      std::string GIT_DIR;
   public:
      // 基本函数
      Repository() = default;
      ~Repository() = default;

      // git函数
      // 注：这里记得各个 string 都要是 const& 格式，否则无法传入字面量
      // Subtask 1
      void init();
      void add(const string& filename);
      void commit(const string& message);
      void rm(const string& filename);

      // Subtask 2
      void log();
      void globalLog(); // 
      void find(const string& target_message); // 

      // Subtask 3 (basic checkout is categorized here for integrity)
      void status();
      void checkoutBranch(const string& branchname);
      void checkoutFile(const string& filename);
      void checkoutFileInCommit(const string& abbr_commit, const string& filename);

      // Subtask 4
      void branch(const string& branchname);
      void rmBranch(const string& branchname);
      void reset(const string commitid);

      // Subtask 5
      void merge(const string& branchname);

      // Subtask 6 (Bonus) will I finish these functions? who knows!
      void addRemote(const string& remotename, const string& remote_dir);
      void rmRemote(const string& remotename);
      void push(const string& remotename, const string& rm_branch);
      void fetch(const string& remotename, const string& rm_branch);
      void pull(const string& remotename, const string& rm_branch);

      // 静态函数
      static std::string getGitliteDir();
};
#endif // REPOSITORY_H
