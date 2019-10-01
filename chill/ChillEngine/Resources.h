// SL 2018-01-05
//------------------------------------------------------------------
#pragma once

//#include <LibSL/LibSL.h>
/*
class Resources
{
  std::string m_Path;

  Resources()
  {
    // search for resources
    std::vector<std::string> paths;
    paths.push_back("./");
    paths.push_back(toPath(std::string(LibSL::System::Application::executablePath())));
    paths.push_back(toPath(std::string(LibSL::System::Application::executablePath()), "..", "icesl-nodes"));
    paths.push_back(toPath(std::string(LibSL::System::Application::executablePath()), "..", "..", "icesl-nodes"));
#ifdef WIN32
    paths.push_back(toPath("C:", "Program Files", "INRIA", "IceSL", "bin"));
    paths.push_back(toPath("C:", "Program Files (x86)", "INRIA", "IceSL", "bin"));
#endif
    bool ok = false;
    ForIndex(i, paths.size()) {
      std::string test = toPath(paths[i],"basic_nodes");
      if (exists(test.c_str())) {
        std::cout << paths[i] << " ------" << std::endl;
        m_Path = paths[i];
        ok = true;
        break;
      }
    }
    if (!ok) {
      throw Fatal("[Resources] cannot find resource files!");
    }
  }

public:

  static std::string toPath(const std::string& first) {
    return first + "";
  }

  template<typename... Args>
  static std::string toPath(const std::string& first, Args... args) {
    return first + '/' + toPath(args...);
  }

  static const std::string& path() 
  { 
    return getInstance().m_Path;
  }

  static Resources& getInstance()
  {
    static Resources r;
    return r;
  }

};
*/

