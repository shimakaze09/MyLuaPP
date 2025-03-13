//
// Created by Admin on 13/03/2025.
//

#include <MyLuaPP/MyLuaPP.h>

using namespace My::MySRefl;
using namespace std;

enum class Color { RED, GREEN, BLUE };

template <>
struct My::MySRefl::TypeInfo<Color> : My::MySRefl::TypeInfoBase<Color> {
  static constexpr AttrList attrs = {};

  static constexpr FieldList fields = {
      Field{"RED", Color::RED},
      Field{"GREEN", Color::GREEN},
      Field{"BLUE", Color::BLUE},
  };
};

int main() {
  char buff[256];
  int error;
  lua_State* L = luaL_newstate(); /* opens Lua */
  luaL_openlibs(L);               /* opens the standard libraries */

  My::MyLuaPP::Register<Color>(L);

  sol::state_view lua(L);
  const char code[] = R"(
 print(Color.RED)
 print(Color.GREEN)
 print(Color.BLUE)
 )";
  cout << code << endl << "----------------------------" << endl;
  lua.script(code);

  while (fgets(buff, sizeof(buff), stdin) != NULL) {
    error = luaL_loadstring(L, buff) || lua_pcall(L, 0, 0, 0);
    if (error) {
      fprintf(stderr, "%s\n", lua_tostring(L, -1));
      lua_pop(L, 1); /* pop error message from the stack */
    }
  }
  lua_close(L);
  return 0;
}
