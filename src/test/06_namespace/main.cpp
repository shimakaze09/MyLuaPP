//
// Created by Admin on 28/03/2025.
//

#include <MyLuaPP/MyLuaPP.h>

#include <iostream>

using namespace My::MySRefl;
using namespace std;

namespace Test {
struct Point {
  float x;
  float y;
};
enum class Color { RED, GREEN, BLUE };
}  // namespace Test

template <>
struct My::MySRefl::TypeInfo<Test::Point> : TypeInfoBase<Test::Point> {
#ifdef MY_MYSREFL_NOT_USE_NAMEOF
  static constexpr char name[12] = "Test::Point";
#endif
  static constexpr AttrList attrs = {};
  static constexpr FieldList fields = {
      Field{TSTR("x"), &Type::x},
      Field{TSTR("y"), &Type::y},
  };
};

template <>
struct My::MySRefl::TypeInfo<Test::Color> : TypeInfoBase<Test::Color> {
#ifdef MY_MYSREFL_NOT_USE_NAMEOF
  static constexpr char name[12] = "Test::Color";
#endif
  static constexpr AttrList attrs = {};
  static constexpr FieldList fields = {
      Field{TSTR("RED"), Type::RED},
      Field{TSTR("GREEN"), Type::GREEN},
      Field{TSTR("BLUE"), Type::BLUE},
  };
};

int main() {
  char buff[256];
  int error;
  lua_State* L = luaL_newstate(); /* opens Lua */
  luaL_openlibs(L);               /* opens the standard libraries */
  My::MyLuaPP::Register<Test::Point>(L);
  My::MyLuaPP::Register<Test::Color>(L);
  {
    sol::state_view lua(L);
    const char code[] = R"(
 p = Test.Point.new()
 p.x = 1
 p.y = 2
 print(p.x, p.y)
 print(Test.Color.RED)
 )";
    cout << code << endl << "----------------------------" << endl;
    lua.safe_script(code);
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
