//
// Created by Admin on 13/03/2025.
//

#include <MyLuaPP/MyLuaPP.h>

#include <iostream>

using namespace My::MySRefl;
using namespace std;

enum class [[meta(520)]] Color {
  RED [[meta("a")]],
  GREEN [[meta("b")]],
  BLUE [[meta("c")]]
};

template <>
struct My::MySRefl::TypeInfo<Color> : TypeInfoBase<Color> {
#ifdef MY_MYSREFL_NOT_USE_NAMEOF
  static constexpr char name[6] = "Color";
#endif
  static constexpr AttrList attrs = {
      Attr{TSTR("meta"), 520},
  };
  static constexpr FieldList fields = {
      Field{TSTR("RED"), Type::RED,
            AttrList{
                Attr{TSTR("meta"), "a"},
            }},
      Field{TSTR("GREEN"), Type::GREEN,
            AttrList{
                Attr{TSTR("meta"), "b"},
            }},
      Field{TSTR("BLUE"), Type::BLUE,
            AttrList{
                Attr{TSTR("meta"), "c"},
            }},
  };
};

int main() {
  char buff[256];
  int error;
  lua_State* L = luaL_newstate(); /* opens Lua */
  luaL_openlibs(L);               /* opens the standard libraries */

  My::MyLuaPP::Register<Color>(L);

  {
    sol::state_view lua(L);
    const char code[] = R"(
print(Color.RED)
print(Color.GREEN)
print(Color.BLUE)
)";
    cout << code << endl << "----------------------------" << endl;
    lua.script(code);
  }

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
