//
// Created by Admin on 29/03/2025.
//

#include <MyLuaPP/MyLuaPP.h>

#include <iostream>

using namespace My::MySRefl;
using namespace std;

struct Point {
  Point(float x, float y) : x{x}, y{y} {}

  Point(Point&&) {}

  Point(const Point&) {}

  float x;
  float y;

  void Func(Point&&) const noexcept { std::cout << "rv" << std::endl; }

  void Func(Point&) const noexcept { std::cout << "lv" << std::endl; }
};

template <>
struct My::MySRefl::TypeInfo<Point> : TypeInfoBase<Point> {
#ifdef MY_MYSREFL_NOT_USE_NAMEOF
  static constexpr char name[6] = "Point";
#endif
  static constexpr AttrList attrs = {};
  static constexpr FieldList fields = {
      Field{TSTR(MyMeta::constructor), WrapConstructor<Type(float, float)>()},
      Field{TSTR(MyMeta::constructor), WrapConstructor<Type(Point&&)>()},
      Field{TSTR(MyMeta::constructor), WrapConstructor<Type(const Point&)>()},
      Field{TSTR("x"), &Type::x},
      Field{TSTR("y"), &Type::y},
      Field{TSTR("Func"),
            static_cast<void (Type::*)(Point&&) const noexcept>(&Type::Func)},
      Field{TSTR("Func"),
            static_cast<void (Type::*)(Point&) const noexcept>(&Type::Func)},
  };
};

int main() {
  auto f = WrapConstructor<Point(Point&&)>();
  using Func = std::decay_t<decltype(f)>;
  using ArgList = My::FuncTraits_ArgList<Func>;
  static_assert(std::is_same_v<ArgList, My::TypeList<Point*, Point&&>>);
  std::is_rvalue_reference_v<My::At_t<ArgList, 1>>;
  constexpr bool crvref = My::MyLuaPP::detail::ContainsRVRef<ArgList>;

  char buff[256];
  int error;
  lua_State* L = luaL_newstate(); /* opens Lua */
  luaL_openlibs(L);               /* opens the standard libraries */
  My::MyLuaPP::Register<Point>(L);
  {
    sol::state_view lua(L);
    const char code[] = R"(
p0 = Point.new(3, 4)
p1 = Point.new(1, 2)
p0:Func(p1) -- lv
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
