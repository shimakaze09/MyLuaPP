```

 __  __       _                ____  ____  
|  \/  |_   _| |   _   _  __ _|  _ \|  _ \ 
| |\/| | | | | |  | | | |/ _` | |_) | |_) |
| |  | | |_| | |__| |_| | (_| |  __/|  __/ 
|_|  |_|\__, |_____\__,_|\__,_|_|   |_|    
        |___/                              

```

# MyLuaPP
My Lua++ (Lua &amp; C++)

Auto register C++ class to Lua with [sol2](https://github.com/ThePhD/sol2) and [MySRefl](https://github.com/shimakaze09/MySRefl) 

## Example

```c++
#include <MyLuaPP/MyLuaPP.h>

using namespace My::MySRefl;
using namespace std;

struct [[size(8)]] Point {
    Point() : x{ 0.f }, y{ 0.f }{}
    Point(float x, float y) : x{ x }, y{ y }{}
    [[not_serialize]]
    float x;
    [[info("hello")]]
    float y;
    float Sum() { return x + y; }
};

template<>
struct TypeInfo<Point> : TypeInfoBase<Point> {
    static constexpr FieldList fields = {
        Field{"constructor", static_cast<void(*)(Point*)>([](Point* p) { new(p)Point; })},
        Field{"constructor",
            static_cast<void(*)(Point*,float,float)>([](Point* p, float x, float y) { new(p)Point{x,y}; })},
        Field{"x", &Point::x, AttrList{ Attr{ "not_serialize" } }},
        Field{"y", &Point::y, AttrList{ Attr{ "info", "hello" } }},
        Field{"Sum", &Point::Sum}
    };

    static constexpr AttrList attrs = {
        Attr{ "size", 8 }
    };
};

int main() {
    char buff[256];
    int error;
    lua_State* L = luaL_newstate(); /* opens Lua */
    luaL_openlibs(L); /* opens the standard libraries */

    My::MyLuaPP::Register<Point>(L);
    sol::state_view lua(L);
    const char code[] = R"(
p0 = Point.new(3, 4)                                       -- constructor
p1 = Point.new()                                           -- constructor overload
print(p0.x, p0.y)                                          -- get field
p1.x = 3                                                   -- set field
print(p1.x, p1.y)
print(p0:Sum())                                             -- non-static member function
print(MySRefl_TypeInfo.Point.attrs.size)                    -- MySRefl type attrs
print(MySRefl_TypeInfo.Point.fields.x.attrs.not_serialize)  -- MySRefl field attrs
print(MySRefl_TypeInfo.Point.fields.y.attrs.info)           -- MySRefl type attrs
)";
    cout << code << endl
        << "----------------------------" << endl;
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
```