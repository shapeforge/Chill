extern "C" {
#include <lua.h>
#include <lualib.h>
}

#include <filesystem>
#include <string>

#include <luabind/luabind.hpp>
#include <luabind/operator.hpp>
#include <luabind/object.hpp>

namespace chill {

#ifdef WIN32
namespace fs = std::experimental::filesystem;
#else
namespace fs = std::filesystem;
#endif

class GraphSaver {
  public:
    GraphSaver() {
      m_LuaState = luaL_newstate();
      luabind::open(m_LuaState);
      registerBindings(m_LuaState);
    }

    ~GraphSaver() {
      lua_close(m_LuaState);
    }
    
    void execute(const fs::path*);

    void registerBindings(lua_State*);

  private:
    lua_State * m_LuaState;
}; // class GraphSaver

} // namespace chill
