#include <LibSL.h>
#include <string>

extern "C" {
#include <lua.h>
#include <lualib.h>
}

#include <luabind/luabind.hpp>
#include <luabind/operator.hpp>
#include <luabind/object.hpp>

namespace Chill
{
  class GraphSaver {
    lua_State * m_LuaState;

  public:
    GraphSaver() {
      m_LuaState = luaL_newstate();
      luabind::open(m_LuaState);
      registerBindings(m_LuaState);
    }
    
    void execute(const char* _path);

    void registerBindings(lua_State *_L);
  };

}