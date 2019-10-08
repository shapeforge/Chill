#pragma once

#include <filesystem>

#include <string>
#include <vector>

namespace chill {
#ifdef WIN32
  namespace fs = std::experimental::filesystem;
#else
  namespace fs = std::filesystem;
#endif

static const std::vector<const char*> OFD_FILTER_GRAPHS = std::vector<const char*>({ "*.graph", "*.lua" });
static const std::vector<const char*> OFD_FILTER_NODES  = std::vector<const char*>({ "*.node" , "*.lua" });
static const std::vector<const char*> OFD_FILTER_LUA    = std::vector<const char*>({ "*.lua" });
static const std::vector<const char*> OFD_FILTER_ALL    = std::vector<const char*>({ "*.*" });

extern fs::path openFileDialog(                                          const std::vector<const char*>* _filter);
extern fs::path openFileDialog(const fs::path* _directory               , const std::vector<const char*>* _filter);
extern fs::path saveFileDialog(const fs::path* _proposedFileNameFullPath, const std::vector<const char*>* _filter);

extern fs::path openFolderDialog(const fs::path* _proposedFolderNameFullPath);

extern std::vector<fs::path> listFolderinDir(const fs::path* _dir);
extern std::vector<fs::path> listFileInDir(const fs::path* _dir, const std::vector<const char*>* _filter);
extern fs::path relative(const fs::path* _absPath, const fs::path* _rootPath); // exists as std::filesystem::relative(path&, base&)

extern fs::path getUserDir();
extern bool isHidden(const fs::path &_p);

extern std::string loadFileIntoString(const fs::path& _path);

}
