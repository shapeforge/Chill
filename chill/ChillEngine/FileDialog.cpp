#include "FileDialog.h"

#include <LibSL/LibSL.h>

#include "../../libs/tinyfiledialogs/tinyfiledialogs.h"


//-------------------------------------------------------

std::string openFileDialog(const std::vector<const char*>& _filter) {
  const char* file = tinyfd_openFileDialog("Open File", nullptr, static_cast<int>(_filter.size()), _filter.data(), nullptr, 0);
  if (file == nullptr) {
    return "";
  } else {
    return std::string(file);
  }
}

//-------------------------------------------------------

std::string openFileDialog(const char* _directory, const std::vector<const char*>& _filter) {
  const char* file = tinyfd_openFileDialog("Open File", _directory, static_cast<int>(_filter.size()), _filter.data(), nullptr, 0);
  if (file == nullptr) {
    return "";
  }
  else {
    return std::string(file);
  }
}

//-------------------------------------------------------

std::string saveFileDialog(const char* _proposedFileNameFullPath, const std::vector<const char*>& _filter) {
  const char* file = tinyfd_saveFileDialog("Save File", _proposedFileNameFullPath, static_cast<int>(_filter.size()), _filter.data(), nullptr);
  if (file == nullptr) {
    return "";
  } else {
    return std::string(file);
  }
}

//-------------------------------------------------------

std::string openFolderDialog(const char* _proposedFolderNameFullPath) {
  const char* folder = tinyfd_selectFolderDialog("Open Folder", _proposedFolderNameFullPath);
  if (folder == nullptr) {
    return "";
  } else {
    return std::string(folder);
  }
}

