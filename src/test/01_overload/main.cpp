//
// Created by Admin on 11/03/2025.
//

#include <MyLuaPP/MyLuaPP.h>

using namespace My::MySRefl;
using namespace std;

struct [[size(8)]] Point {};

template <>
struct My::MySRefl::TypeInfo<Point> : My::MySRefl::TypeInfoBase<Point> {
  static constexpr AttrList attrs = {
      Attr{"size", 8},
  };

  static constexpr FieldList fields = {
      Field{"x", &Point::x,
            AttrList{
                Attr{"not_serialize"},
            }},
      Field{"y", &Point::y,
            AttrList{
                Attr{"info", "hello"},
            }},
      Field{Name::constructor, WrapConstructor<Point()>()},
      Field{Name::constructor, WrapConstructor<Point(float, float)>(),
            AttrList{
                Attr{MY_MYSREFL_NAME_ARG(0),
                     AttrList{
                         Attr{Name::name, "x"},
                     }},
                Attr{MY_MYSREFL_NAME_ARG(1),
                     AttrList{
                         Attr{Name::name, "y"},
                     }},
            }},
      Field{"Sum", static_cast<float (Point::*)()>(&Point::Sum)},
      Field{"Sum", static_cast<float (Point::*)(float)>(&Point::Sum),
            AttrList{
                Attr{MY_MYSREFL_NAME_ARG(0),
                     AttrList{
                         Attr{Name::name, "z"},
                     }},
            }},
      Field{"Min", static_cast<float (Point::*)()>(&Point::Min),
            AttrList{
                Attr{"number", 1024},
            }},
      Field{"Min", static_cast<float (Point::*)(float)>(&Point::Min),
            AttrList{
                Attr{"number", 520},
                Attr{MY_MYSREFL_NAME_ARG(0),
                     AttrList{
                         Attr{Name::name, "z"},
                     }},
            }},
      Field{"Add", &Point::Add,
            AttrList{
                Attr{MY_MYSREFL_NAME_ARG(0),
                     AttrList{
                         Attr{Name::name, "rhs"},
                     }},
            }},
      Field{"operator-", &Point::operator-,
            AttrList{
                Attr{MY_MYSREFL_NAME_ARG(0),
                     AttrList{
                         Attr{Name::name, "rhs"},
                     }},
            }},
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
print(MySRefl_TypeInfo.Point.attrs.size)                    -- MySRefl type attrs
print(MySRefl_TypeInfo.Point.fields.x.attrs.not_serialize)  -- MySRefl field attrs
print(MySRefl_TypeInfo.Point.fields.y.attrs.info)           -- MySRefl type attrs
print(p0:Sum(2))                                           -- non-static member function overload
print(p0:Min())                                            -- non-static member function overload
print(p0:Min(-1))                                          -- non-static member function overload
print(MySRefl_TypeInfo.Point.fields.Min_0.attrs.number)     -- non-static member function overload
print(MySRefl_TypeInfo.Point.fields.Min_1.attrs.number)     -- non-static member function overload
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
