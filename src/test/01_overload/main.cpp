//
// Created by Admin on 11/03/2025.
//

#include <MyLuaPP/MyLuaPP.h>

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
  float Min() { return x < y ? x : y; }
  float Min(float z) { return x < y ? (x < z ? x : z) : (y < z ? y : z); }
};

template <>
struct TypeInfo<Point> : TypeInfoBase<Point> {
  static constexpr FieldList fields = {
      Field{"constructor",
            static_cast<void (*)(Point*)>([](Point* p) { new (p) Point; })},
      Field{"constructor",
            static_cast<void (*)(Point*, float, float)>(
                [](Point* p, float x, float y) { new (p) Point{x, y}; })},
      Field{"x", &Point::x, AttrList{Attr{"not_serialize"}}},
      Field{"y", &Point::y, AttrList{Attr{"info", "hello"}}},
      Field{"Sum", static_cast<float (Point::*)()>(&Point::Sum)},
      Field{"Sum", static_cast<float (Point::*)(float)>(&Point::Sum)},
      Field{"Min", static_cast<float (Point::*)()>(&Point::Min)},
      Field{"Min", static_cast<float (Point::*)(float)>(&Point::Min)}};

  static constexpr AttrList attrs = {Attr{"size", 8}};
};

int main() {
  char buff[256];
  int error;
  lua_State* L = luaL_newstate(); /* opens Lua */
  luaL_openlibs(L);               /* opens the standard libraries */

  My::MyLuaPP::Register<Point>(L);
  sol::state_view lua(L);
  const char code[] = R"(
 p0 = Point.new(3, 4)                                       -- constructor
 p1 = Point.new()                                           -- constructor overload
 print(p0.x, p0.y)                                          -- get field
 p1.x = 3                                                   -- set field
 print(p1.x, p1.y)
 print(p0:Sum())                                            -- non-static member function
 print(MySRefl_TypeInfo.Point.attrs.size)                    -- MySRefl type attrs
 print(MySRefl_TypeInfo.Point.fields.x.attrs.not_serialize)  -- MySRefl field attrs
 print(MySRefl_TypeInfo.Point.fields.y.attrs.info)           -- MySRefl type attrs
 print(p0:Sum(2))                                           -- non-static member function overload
 print(p0:Min())                                            -- non-static member function overload
 print(p0:Min(-1))                                          -- non-static member function overload
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
