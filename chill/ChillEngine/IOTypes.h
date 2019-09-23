#pragma once

#include <string>
#include <sstream>

// Search and remove whitespace from both ends of the string
static std::string TrimEnumString(const std::string &_s)
{
  std::string::const_iterator it = _s.begin();
  while (it != _s.end() && isspace(*it)) { it++; }
  std::string::const_reverse_iterator rit = _s.rbegin();
  while (rit.base() != it && isspace(*rit)) { rit++; }
  return std::string(it, rit.base());
}

static void SplitEnumArgs(const char* _szArgs, std::string _Array[], int _nMax)
{
  std::stringstream ss(_szArgs);
  std::string strSub;
  int nIdx = 0;
  while (ss.good() && (nIdx < _nMax)) {
    getline(ss, strSub, ',');
    _Array[nIdx] = TrimEnumString(strSub);
    nIdx++;
  }
};
// This will to define an enum that is wrapped in a namespace of the same name along with ToString(), FromString(), and COUNT
#define DECLARE_ENUM(ename, ...) \
    namespace ename { \
        enum ename { __VA_ARGS__, COUNT }; \
        static std::string _Strings[COUNT]; \
        static const char* ToString(ename e) { \
            if (_Strings[0].empty()) { SplitEnumArgs(#__VA_ARGS__, _Strings, COUNT); } \
            return _Strings[e].c_str(); \
        } \
        static ename FromString(const std::string& strEnum) { \
            if (_Strings[0].empty()) { SplitEnumArgs(#__VA_ARGS__, _Strings, COUNT); } \
            for (int i = 0; i < COUNT; i++) { if (_Strings[i] == strEnum) { return static_cast<ename>(i); } } \
            return static_cast<ename>(0); \
        } \
    }

DECLARE_ENUM(IOType, UNDEF, BOOLEAN, IMPLICIT, INTEGER, LIST, PATH, REAL, STRING, SHAPE, FIELD, VEC3, VEC4)

namespace IOType {
  static bool isCompatible(IOType _typeOutput, IOType _typeInput) {
    if (_typeOutput == _typeInput)
      return true;
    if (_typeOutput == IOType::UNDEF || _typeInput == IOType::UNDEF)
      return true;
    if (_typeOutput == IOType::VEC4 && (_typeInput == IOType::VEC3 || _typeInput == IOType::REAL))
      return true;
    if (_typeOutput == IOType::VEC3 && _typeInput == IOType::REAL)
      return true;
    if (_typeOutput == IOType::STRING && _typeInput == IOType::PATH)
      return true;
    return false;
  }
}
