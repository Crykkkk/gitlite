#include "../include/Blob.h"
#include "../include/Repository.h"

Blob::Blob(string& filename) {
   target_file = filename;
   string target_path = Utils::join(Repository::getWorkingDir(), target_file); // 注意这里文件都是单层的
   if (!Utils::isFile(target_path)) Utils::exitWithMessage("File does not exist.");
   content = Utils::readContents(target_path);
}
