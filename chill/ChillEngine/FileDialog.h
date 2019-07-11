#pragma once

#include <string>

#ifdef WIN32
#define OFD_FILTER_GRAPHS     "Graphs (*.graph, *.lua)\0*.graph;*.lua\0All (*.*)\0*.*\0"
#define OFD_FILTER_NODES      "Nodes  (*.node, *.lua)\0*.node;*.lua\0All (*.*)\0*.*\0"
#define OFD_FILTER_LUA        "Lua    (*.lua)\0*.lua\0All (*.*)\0*.*\0"
#define OFD_FILTER_NONE       "All    (*.*)\0*.*\0"
#else
#define OFD_FILTER_GRAPHS     std::vector<const char*>({"*.graph", "*.lua"})
#define OFD_FILTER_NODES      std::vector<const char*>({"*.node" , "*.lua"})
#define OFD_FILTER_LUA      std::vector<const char*>({"*.lua"})
#define OFD_FILTER_NONE       std::vector<const char*>({"*.*"})
#endif

#ifdef WIN32
std::string openFileDialog(const char* filter);
std::string openFileDialog(const char* directory, const char* filter);
std::string saveFileDialog(const char* proposedFileNameFullPath, const char* filter);
#else
std::string openFileDialog(std::vector<const char*> filter);
std::string openFileDialog(const char* directory, std::vector<const char*> filter);
std::string saveFileDialog(const char* proposedFileNameFullPath, std::vector<const char*> filter);
#endif

std::string openFolderDialog(const char* proposedFolderNameFullPath);
