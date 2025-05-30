//
// Created by Admin on 30/03/2025.
//

#include <MyLuaPP/MyLuaPP.h>
#include <iostream>
#include "Span.h"

using namespace My::MySRefl;
using namespace std;

struct Point {
  Point() : x{0.f}, y{0.f} {}

  Point(float x, float y) : x{x}, y{y} {}

  float x;
  float y;

  My::Span<const float> GetSpanX() const { return {&x, 1}; }

  void SetSpanX(My::Span<const float> sx) { x = sx[0]; }
};

template <>
struct My::MySRefl::TypeInfo<Point> : TypeInfoBase<Point> {
#ifdef MY_MYSREFL_NOT_USE_NAMEOF
  static constexpr char name[6] = "Point";
#endif
  static constexpr AttrList attrs = {};
  static constexpr FieldList fields = {
      Field{TSTR(MyMeta::constructor), WrapConstructor<Type()>()},
      Field{TSTR(MyMeta::constructor), WrapConstructor<Type(float, float)>()},
      Field{TSTR("x"), &Type::x},
      Field{TSTR("y"), &Type::y},
      Field{TSTR("GetSpanX"), &Type::GetSpanX},
      Field{TSTR("SetSpanX"), &Type::SetSpanX},
  };
};

int main() {
  char buff[256];
  int error;
  lua_State* L = luaL_newstate(); /* opens Lua */
  luaL_openlibs(L);               /* opens the standard libraries */
  My::MyLuaPP::Register<Point>(L);
  {
    sol::state_view lua(L);
    const char code[] = R"(
p0 = Point.new(3, 4)
p1 = Point.new()
s = p0:GetSpanX()
print(p1.x)
p1:SetSpanX(s)
print(p1.x)
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
