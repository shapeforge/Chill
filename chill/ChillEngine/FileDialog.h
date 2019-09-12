#pragma once

#include <string>
#include <vector>

static const std::vector<const char*> OFD_FILTER_GRAPHS = std::vector<const char*>({ "*.graph", "*.lua" });
static const std::vector<const char*> OFD_FILTER_NODES  = std::vector<const char*>({ "*.node" , "*.lua" });
static const std::vector<const char*> OFD_FILTER_LUA    = std::vector<const char*>({ "*.lua" });
static const std::vector<const char*> OFD_FILTER_ALL    = std::vector<const char*>({ "*.*" });

std::string openFileDialog(                                      const std::vector<const char*>& filter);
std::string openFileDialog(const char* directory               , const std::vector<const char*>& filter);
std::string saveFileDialog(const char* proposedFileNameFullPath, const std::vector<const char*>& filter);

std::string openFolderDialog(const char* proposedFolderNameFullPath);
