#include "../include/Blob.h"
#include "../include/Repository.h"

Blob::Blob(const string& filename) {
   target_file = filename;
   string target_path = Utils::join(Repository::getWorkingDir(), target_file); // 注意这里文件都是单层的
   if (!Utils::isFile(target_path)) Utils::exitWithMessage("File does not exist.");
   content = Utils::readContents(target_path);
   Hash = Utils::sha1(this->content);
}

void Blob::save_blob(){
   string blob_path = Utils::join(Repository::getBlobsDir(), Hash);
   Utils::writeContents(blob_path, content);
}

string Blob::blob_deserial_content(const string& blobhash) {
   string blob_path = Utils::join(Repository::getBlobsDir(), blobhash);
   string file_content = Utils::readContentsAsString(blob_path);
   return file_content;
}