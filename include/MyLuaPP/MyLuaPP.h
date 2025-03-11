#pragma once

#include <MySRefl/MySRefl.h>
#include <MyTemplate/Typelist.h>

#include <array>
#include <lua.hpp>
#include <string>

#include "_deps/sol2/sol.hpp"

namespace My::MyLuaPP {
namespace detail {
struct NameInfo {
  NameInfo(std::string_view name) {
    size_t cur = name[0] == 's' ? 6 : 5;
    std::string str;
    while (++cur < name.size()) {
      if (name[cur] == ':') {
        namespaces.push_back(str);
        str.clear();
        cur++;
      } else if (isalnum(name[cur]) || name[cur] == '_')
        str += name[cur];
      else
        str += '_';
    }
    rawName = str;
  }

  std::vector<std::string> namespaces;
  std::string rawName;
};

template <typename T, size_t... Ns>
constexpr auto GetInits(std::index_sequence<Ns...>) {
  constexpr auto masks = My::MySRefl::TypeInfo<T>::fields.Accumulate(
      std::array<bool, My::MySRefl::TypeInfo<T>::fields.size>{},
      [idx = 0](auto&& acc, auto field) mutable {
        acc[idx++] = field.name == "constructor";
        return std::forward<decltype(acc)>(acc);
      });
  constexpr auto constructors =
      My::MySRefl::TypeInfo<T>::fields.Accumulate<masks[Ns]...>(
          std::tuple<>{}, [](auto acc, auto field) {
            return std::tuple_cat(acc, std::tuple{field.value});
          });
  return std::apply([](auto... elems) { return sol::initializers(elems...); },
                    constructors);
}
}  // namespace detail

template <typename T>
void Register(lua_State* L) {
  sol::state_view lua(L);
  sol::table typeinfo = lua["MySRefl_TypeInfo"].get_or_create<sol::table>();
  detail::NameInfo nameInfo(MySRefl::TypeInfo<T>::name);

  sol::usertype<T> type = lua.new_usertype<T>(
      nameInfo.rawName,
      detail::GetInits<T>(
          std::make_index_sequence<My::MySRefl::TypeInfo<T>::fields.size>{}));
  MySRefl::TypeInfo<T>::DFS([&](auto t, size_t) {
    t.fields.ForEach([&](auto field) { type[field.name] = field.value; });
  });
  sol::table typeinfo_type =
      typeinfo[nameInfo.rawName].get_or_create<sol::table>();
  sol::table typeinfo_type_attrs =
      typeinfo_type["attrs"].get_or_create<sol::table>();
  sol::table typeinfo_type_fields =
      typeinfo_type["fields"].get_or_create<sol::table>();
  MySRefl::TypeInfo<T>::attrs.ForEach([&](auto attr) {
    if constexpr (attr.has_value)
      typeinfo_type_attrs[attr.name] = attr.value;
    else
      typeinfo_type_attrs[attr.name] = true;  // default
  });
  MySRefl::TypeInfo<T>::DFS([&](auto t, size_t) {
    t.fields.ForEach([&](auto field) {
      sol::table typeinfo_type_fields_field =
          typeinfo_type_fields[field.name].get_or_create<sol::table>();
      sol::table typeinfo_type_fields_field_attrs =
          typeinfo_type_fields_field["attrs"].get_or_create<sol::table>();
      if constexpr (field.attrs.size > 0) {
        field.attrs.ForEach([&](auto attr) {
          if constexpr (attr.has_value)
            typeinfo_type_fields_field_attrs[attr.name] = attr.value;
          else
            typeinfo_type_fields_field_attrs[attr.name] = true;  // default
        });
      }
    });
  });
}
}  // namespace My::MyLuaPP