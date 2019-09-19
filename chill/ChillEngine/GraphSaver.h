extern "C" {
#include <lua.h>
#include <lualib.h>
}

#include <string>

#include <LibSL.h>
#include <luabind/luabind.hpp>
#include <luabind/operator.hpp>
#include <luabind/object.hpp>

namespace chill {

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
    
    void execute(const char*);

    void registerBindings(lua_State*);

  private:
    lua_State * m_LuaState;
}; // class GraphSaver

} // namespace chill
