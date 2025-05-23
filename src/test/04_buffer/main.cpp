//
// Created by Admin on 13/03/2025.
//

#include <MyLuaPP/MyLuaPP.h>

#include <iostream>

using namespace My::MySRefl;
using namespace std;

class Buffer {
 public:
  Buffer(size_t s) {
    buffer = malloc(s);
    cout << "malloc buffer @" << buffer << endl;
  }

  ~Buffer() {
    cout << "free buffer @" << buffer << endl;
    free(buffer);
  }

  double& GetNumber(size_t offset) {
    return *(double*)((uint8_t*)buffer + offset);
  }

  void SetNumber(size_t offset, double value) { GetNumber(offset) = value; }

  void InitTable(size_t offset) { new ((uint8_t*)buffer + offset) sol::table; }

  void InitTable(size_t offset, sol::table value) {
    new ((uint8_t*)buffer + offset) sol::table(move(value));
  }

  sol::table& GetTable(size_t offset) {
    return *(sol::table*)((uint8_t*)buffer + offset);
  }

  void SetTable(size_t offset, sol::table value) { GetTable(offset) = value; }

 private:
  void* buffer;
};

template <>
struct My::MySRefl::TypeInfo<Buffer> : TypeInfoBase<Buffer> {
#ifdef MY_MYSREFL_NOT_USE_NAMEOF
  static constexpr char name[7] = "Buffer";
#endif
  static constexpr AttrList attrs = {};
  static constexpr FieldList fields = {
      Field{TSTR(MyMeta::constructor), WrapConstructor<Type(size_t)>()},
      Field{TSTR(MyMeta::destructor), WrapDestructor<Type>()},
      Field{TSTR("GetNumber"), &Type::GetNumber},
      Field{TSTR("SetNumber"), &Type::SetNumber},
      Field{TSTR("InitTable"),
            static_cast<void (Type::*)(size_t)>(&Type::InitTable)},
      Field{TSTR("InitTable"),
            static_cast<void (Type::*)(size_t, sol::table)>(&Type::InitTable)},
      Field{TSTR("GetTable"), &Type::GetTable},
      Field{TSTR("SetTable"), &Type::SetTable},
  };
};

constexpr int f() {
  int a = 1;
  return a;
}

int main() {
  char buff[256];
  int error;
  lua_State* L = luaL_newstate(); /* opens Lua */
  luaL_openlibs(L);               /* opens the standard libraries */
  My::MyLuaPP::Register<Buffer>(L);
  {
    sol::state_view lua(L);
    const char code[] = R"(
buf = Buffer.new(32) -- 2 double + 1 table
buf:SetNumber(0, 512)
buf:SetNumber(8, 1024)
print(buf:GetNumber(0))
print(buf:GetNumber(8))
buf:InitTable(16, {hello="world"})
t = buf:GetTable(16)
t["Lua"] = "nice"
for k,v in pairs(buf:GetTable(16)) do print(k, v) end
t = nil
for k,v in pairs(buf:GetTable(16)) do print(k, v) end
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
