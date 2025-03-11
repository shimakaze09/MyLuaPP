#pragma once

#include <lua.hpp>

namespace My::MyLuaPP {
template <typename T>
void Register(lua_State* L);
}  // namespace My::MyLuaPP

#include "details/MyLuaPP.inl"
