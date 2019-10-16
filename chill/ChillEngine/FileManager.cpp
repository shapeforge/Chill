#include "FileManager.h"

#include <iostream>
#include <sstream>
#include <fstream>

#include <tinyfiledialogs.h>

#include "SourcePath.h"


namespace chill {
  //-------------------------------------------------------

  fs::path openFileDialog(const std::vector<const char*>* _filter) {
    const char* file = tinyfd_openFileDialog("Open File", nullptr, static_cast<int>(_filter->size()), _filter->data(), nullptr, 0);
    if (!file) {
      file = "";
    }
    return fs::path(file);
  }

  //-------------------------------------------------------

  fs::path openFileDialog(const fs::path* _directory, const std::vector<const char*>* _filter) {
    const char* file = tinyfd_openFileDialog("Open File", _directory->generic_string().c_str(), static_cast<int>(_filter->size()), _filter->data(), nullptr, 0);
    if (!file) {
      file = "";
    }
    return fs::path(file);
  }

  //-------------------------------------------------------

  fs::path saveFileDialog(const fs::path* _proposedFileNameFullPath, const std::vector<const char*>* _filter) {
    const char* file = tinyfd_saveFileDialog("Save File", _proposedFileNameFullPath->generic_string().c_str(), static_cast<int>(_filter->size()), _filter->data(), nullptr);
    if (!file) {
      file = "";
    }
    return fs::path(file);
  }

  //-------------------------------------------------------

  fs::path openFolderDialog(const fs::path* _proposedFolderNameFullPath) {
    const char* folder = tinyfd_selectFolderDialog("Open Folder", _proposedFolderNameFullPath->generic_string().c_str());
    if (!folder) {
      folder = "";
    }
    return fs::path(folder);
  }

  //-------------------------------------------------------
  std::vector<fs::path> listFolderinDir(const fs::path* _dir)
  {
    fs::path dir(_dir->generic_string());
    std::vector<fs::path> paths;

    if(!fs::exists(dir))
      return paths;

    for (auto& p : fs::directory_iterator(dir)) {
      fs::path folder = p;
      if (is_directory(folder)) {
        paths.push_back(folder);
      }
    }
    return paths;
  }

  std::vector<fs::path> listFileInDir(const fs::path* _dir, const std::vector<const char*>* _filter)
  {
    fs::path dir(_dir->generic_string());
    std::vector<fs::path> paths;

    if(!fs::exists(dir))
      return paths;

    for (auto& p : fs::directory_iterator(dir)) {
      if (is_regular_file(p)) {

        for (auto filter : *_filter) {
          if (p.path().extension().generic_string() == std::string(filter).erase(0,1)) {
            paths.push_back(p);
          }
        }
      }
    }
    return paths;
  }

  //-------------------------------------------------------

  // exists as std::filesystem::relative(path&, base&)
  fs::path relative(const fs::path* _absPath, const fs::path* _rootPath) {
    std::string root = _rootPath->generic_string();
    std::string abslt = _absPath->generic_string();
    unsigned long nfsize = static_cast<unsigned long>(root.size());
    if (abslt[nfsize + 1] == '/') nfsize++;
    return fs::path(abslt.substr(nfsize));
  }

  //-------------------------------------------------------
  fs::path getUserDir()
  {
    fs::path         path;
    std::vector<fs::path> paths;

#ifdef WIN32
    paths.push_back(getenv("APPDATA") + std::string("/ChiLL/"));
#elif UNIX
    paths.push_back("/etc/chill/");
#endif
    paths.push_back(SRC_PATH); // dev path
    paths.push_back("../ChiLL");
    paths.push_back("../../ChiLL");
    paths.push_back("../../../ChiLL");
    paths.push_back("../../../../ChiLL");

    for (auto p : paths) {
      if (fs::exists(p)) {
        return p;
      }
    }
    return fs::current_path();
  }

  bool isHidden(const fs::path &p)
  {
    fs::path::string_type name = p.filename();
    return name[0] == '.';
  }


  std::string loadFileIntoString(const fs::path& _path) {
    auto ss = std::ostringstream{};
    std::ifstream file(_path);
    ss << file.rdbuf();
    return ss.str();
  }
}
