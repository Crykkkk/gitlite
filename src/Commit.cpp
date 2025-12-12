#include "../include/Commit.h"
#include "../include/Repository.h"
#include <cstddef>
#include <sstream>
#include <string>
#include <vector>
// p.s. 如果只需要用静态函数就在cpp include，在头文件include会循环引用

Commit::Commit(string father, string mess):father_hash(father), message(mess), second_parent_hash(""), Hash(""){
   if (father_hash == "") {
      time_stamp = 0;
      file_blob_map = {};
      // 这个阶段还不需要 hash，可以save才算出来
   }
   else {
      time_stamp = std::time(nullptr);
      file_blob_map = commit_deserial(father).file_blob_map;
      // TODO: 根据 staging area 更新 file_blob_map
   }
}

std::map<string, string>& Commit::check_map(){
   return file_blob_map;
}

void Commit::save_commit(){
   std::vector<unsigned char> serial_cm = commit_serial(*this);
   Hash = Utils::sha1(serial_cm);
   string commit_path = Utils::join(Repository::getCommitsDir(), Hash);
   Utils::writeContents(commit_path, serial_cm);
   string head = Repository::getHeadbranch();
   string target_path = Utils::join(Repository::getBranchesDir(), head);
   Utils::writeContents(target_path, Hash);
}

std::vector<unsigned char> Commit::commit_serial(const Commit& cmt) { // 只计算，不写入
   std::ostringstream oss;
   oss << "---Metadata---" << '\n';
   oss << cmt.father_hash << '\n';
   oss << cmt.second_parent_hash << '\n';
   oss << cmt.message << '\n';
   oss << cmt.time_stamp << '\n';
   oss << "---Files---" << '\n';
   for (auto i : cmt.file_blob_map) {
      oss << i.first << ' ' << i.second << '\n';
   }
   return Utils::serialize(oss.str());
}

Commit Commit::commit_deserial(const std::string Hs){
   Commit cm;
   string target_commit = Utils::join(Repository::getCommitsDir(), Hs);
   std::istringstream iss(Utils::readContentsAsString(target_commit));
   string line;
   // 逐步生成信息
   // metadata部分
   std::getline(iss, line);
   std::getline(iss, line);
   cm.father_hash = line;
   std::getline(iss, line);
   cm.second_parent_hash = line;
   std::getline(iss, line);
   cm.message = line;
   std::getline(iss, line);
   cm.time_stamp = std::stoll(line);
   std::getline(iss, line);

   // filedata部分
   while (std::getline(iss, line)) {
      if (line.empty()) continue;
      
      std::stringstream lineSS(line);
      std::string filename, hash;
      lineSS >> filename >> hash;
      if (!filename.empty()) {
         cm.file_blob_map[filename] = hash;
      }
   }
   // hash本身
   cm.Hash = Hs;
   return cm;
}

void Commit::show() {
   std::cout << "===" << std::endl;
   std::cout << "commit ";
   if (Hash == "") {
      std::vector<unsigned char> serial_cm = commit_serial(*this);
      Hash = Utils::sha1(serial_cm);
   }
   std::cout << Hash << std::endl;
   if (second_parent_hash != "") {
      std::cout << "Merge: " << father_hash.substr(0,7) << ' ' << second_parent_hash.substr(0, 7) << std::endl;
   }
   std::cout << "Date: " << Utils::format_time(time_stamp) << std::endl;
   std::cout << message << std::endl;
}