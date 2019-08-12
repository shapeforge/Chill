#include "../../libs/tinyfiledialogs/tinyfiledialogs.h"

#include <LibSL/LibSL.h>

#include "FileDialog.h"

std::string openFileDialog(const std::vector<const char*>& filter)
{
  const char* file = tinyfd_openFileDialog("Open File", NULL, filter.size(), filter.data(), NULL, 0);
  if(file == NULL) {
    return "";
  } else {
    return std::string(file);
  }
}

std::string openFileDialog(const char* directory, const std::vector<const char*>& filter)
{
  const char* file = tinyfd_openFileDialog("Open File", directory, filter.size(), filter.data(), NULL, 0);
  if (file == NULL) {
    return "";
  }
  else {
    return std::string(file);
  }
}

std::string saveFileDialog(const char* proposedFileNameFullPath, const std::vector<const char*>& filter)
{
  const char* file = tinyfd_saveFileDialog("Save File", proposedFileNameFullPath, filter.size(), filter.data(), NULL);
  if(file == NULL) {
    return "";
  } else {
    return std::string(file);
  }
}


std::string openFolderDialog(const char* proposedFolderNameFullPath)
{
  const char* folder = tinyfd_selectFolderDialog("Open Folder", proposedFolderNameFullPath);
  if(folder == NULL) {
    return "";
  } else {
    return std::string(folder);
  }
}

