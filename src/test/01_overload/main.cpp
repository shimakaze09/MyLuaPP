//
// Created by Admin on 11/03/2025.
//

#include <MyLuaPP/MyLuaPP.h>

#include <iostream>

using namespace My::MySRefl;
using namespace std;

struct [[size(8)]] Point {
  Point() : x{0.f}, y{0.f} {}

  Point(float x, float y) : x{x}, y{y} {}

  [[not_serialize]]
  float x;
  [[info("hello")]]
  float y;

  float Sum() { return x + y; }

  float Sum(float z) { return x + y + z; }

  [[number(1024)]]
  float Min() {
    return x < y ? x : y;
  }

  [[number(520)]]
  float Min(float z) {
    return x < y ? (x < z ? x : z) : (y < z ? y : z);
  }

  Point Add(const Point& rhs) const { return {x + rhs.x, y + rhs.y}; }

  Point operator-(const Point& rhs) const { return {x - rhs.x, y - rhs.y}; }
};

template <>
struct My::MySRefl::TypeInfo<Point> : TypeInfoBase<Point> {
#ifdef MY_MYSREFL_NOT_USE_NAMEOF
  static constexpr char name[6] = "Point";
#endif
  static constexpr AttrList attrs = {
      Attr{TSTR("size"), 8},
  };
  static constexpr FieldList fields = {
      Field{TSTR(MyMeta::constructor), WrapConstructor<Type()>()},
      Field{TSTR(MyMeta::constructor), WrapConstructor<Type(float, float)>()},
      Field{TSTR("x"), &Type::x,
            AttrList{
                Attr{TSTR("not_serialize")},
            }},
      Field{TSTR("y"), &Type::y,
            AttrList{
                Attr{TSTR("info"), "hello"},
            }},
      Field{TSTR("Sum"), static_cast<float (Type::*)()>(&Type::Sum)},
      Field{TSTR("Sum"), static_cast<float (Type::*)(float)>(&Type::Sum)},
      Field{TSTR("Min"), static_cast<float (Type::*)()>(&Type::Min),
            AttrList{
                Attr{TSTR("number"), 1024},
            }},
      Field{TSTR("Min"), static_cast<float (Type::*)(float)>(&Type::Min),
            AttrList{
                Attr{TSTR("number"), 520},
            }},
      Field{TSTR("Add"), &Type::Add},
      Field{
          TSTR("operator-"),
          &Type::operator- },
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
p0 = Point.new(3, 4)                                       -- constructor
p1 = Point.new()                                           -- constructor overload
print(p0.x, p0.y)                                          -- get field
p1.x = 3                                                   -- set field
print(p1.x, p1.y)
print(p0:Sum())                                            -- non-static member function
--print(MySRefl_TypeInfo.Point.attrs.size)                    -- MySRefl type attrs
--print(MySRefl_TypeInfo.Point.fields.x.attrs.not_serialize)  -- MySRefl field attrs
--print(MySRefl_TypeInfo.Point.fields.y.attrs.info)           -- MySRefl type attrs
print(p0:Sum(2))                                           -- non-static member function overload
print(p0:Min())                                            -- non-static member function overload
print(p0:Min(-1))                                          -- non-static member function overload
--print(MySRefl_TypeInfo.Point.fields.Min_0.attrs.number)     -- non-static member function overload
--print(MySRefl_TypeInfo.Point.fields.Min_1.attrs.number)     -- non-static member function overload
p2 = p0:Add(p1)                                            -- usertype argument
print(p2.x, p2.y)
p3 = p0 - p1                                               -- meta function
print(p3.x, p3.y)
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
