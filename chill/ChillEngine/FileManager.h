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

static fs::path openFileDialog(                                          const std::vector<const char*>* filter);
static fs::path openFileDialog(const fs::path* directory               , const std::vector<const char*>* filter);
static fs::path saveFileDialog(const fs::path* proposedFileNameFullPath, const std::vector<const char*>* filter);

static fs::path openFolderDialog(const fs::path* proposedFolderNameFullPath);

static std::vector<fs::path> listFolderinDir(const fs::path* _dir);
static std::vector<fs::path> listFileInDir(const fs::path* _dir, const std::vector<const char*>* filter);
static fs::path relative(const fs::path* _absPath, const fs::path* _rootPath); // exists as std::filesystem::relative(path&, base&)

static fs::path getUserDir();
static bool isHidden(const fs::path &p);

}
