#include "../include/Index.h"
#include "../include/Repository.h"
#include <sstream>
#include <string>

void Index::writeToDisk(){
   std::ostringstream ossa;
   for (auto i : added) {
      ossa << i.first << ' ' << i.second << '\n';
   }
   std::ostringstream ossr;
   for (auto i : removed) {
      ossr << i << '\n';
   }
   string add_path = Utils::join(Repository::getIndexDir(), "add");
   string rm_path = Utils::join(Repository::getIndexDir(), "remove");
   Utils::writeContents(add_path, ossa.str());
   Utils::writeContents(rm_path, ossr.str());
}

void Index::readFromDisk() {
   string add_path = Utils::join(Repository::getIndexDir(), "add");
   string rm_path = Utils::join(Repository::getIndexDir(), "remove");
   
   if (Utils::isFile(add_path)) {
      std::istringstream issa(Utils::readContentsAsString(add_path));
      string linea;
      while (std::getline(issa, linea)) {
         std::stringstream lineass(linea);
         string fname, bhash;
         lineass >> fname >> bhash;
         added[fname] = bhash;
      }
   }
   else added.clear();

   if (Utils::isFile(rm_path)) {
      std::istringstream issr(Utils::readContentsAsString(rm_path));
      string liner;
      while (std::getline(issr, liner)) {
         removed.insert(liner);
      }
   }
   else removed.clear();
}

