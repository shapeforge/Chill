#include "../../libs/tinyfiledialogs/tinyfiledialogs.h"

#include <LibSL/LibSL.h>

#include "FileDialog.h"

std::string openFileDialog(const std::vector<const char*>& _filter)
{
  const char* file = tinyfd_openFileDialog("Open File", NULL, static_cast<int>(_filter.size()), _filter.data(), NULL, 0);
  if(file == NULL) {
    return "";
  } else {
    return std::string(file);
  }
}

std::string openFileDialog(const char* _directory, const std::vector<const char*>& _filter)
{
  const char* file = tinyfd_openFileDialog("Open File", _directory, static_cast<int>(_filter.size()), _filter.data(), NULL, 0);
  if (file == NULL) {
    return "";
  }
  else {
    return std::string(file);
  }
}

std::string saveFileDialog(const char* _proposedFileNameFullPath, const std::vector<const char*>& _filter)
{
  const char* file = tinyfd_saveFileDialog("Save File", _proposedFileNameFullPath, static_cast<int>(_filter.size()), _filter.data(), NULL);
  if(file == NULL) {
    return "";
  } else {
    return std::string(file);
  }
}


std::string openFolderDialog(const char* _proposedFolderNameFullPath)
{
  const char* folder = tinyfd_selectFolderDialog("Open Folder", _proposedFolderNameFullPath);
  if(folder == NULL) {
    return "";
  } else {
    return std::string(folder);
  }
}

