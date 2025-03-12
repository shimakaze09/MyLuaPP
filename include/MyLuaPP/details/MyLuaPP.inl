//
// Created by Admin on 11/03/2025.
//

#pragma once

#include <MySRefl/MySRefl.h>

#include <array>
#include <string>

#include "_deps/sol2/sol.hpp"

namespace My::MyLuaPP::detail {
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
      My::MySRefl::TypeInfo<T>::fields.template Accumulate<masks[Ns]...>(
          std::tuple<>{}, [](auto acc, auto field) {
            return std::tuple_cat(acc, std::tuple{field.value});
          });
  return std::apply([](auto... elems) { return sol::initializers(elems...); },
                    constructors);
}

template <typename T>
constexpr auto GetFuncNumOverloadNum() {
  constexpr auto funcFields = My::MySRefl::TypeInfo<T>::DFS_Acc(
      MySRefl::ElemList<>{}, [](auto acc, auto baseType, size_t) {
        return baseType.fields.Accumulate(acc, [](auto acc, auto field) {
          if constexpr (field.is_func)
            return acc.Push(field);
          else
            return acc;
        });
      });

  constexpr auto overloadNames = funcFields.Accumulate(
      std::array<std::string_view, funcFields.size>{},
      [idx = static_cast<size_t>(0)](auto acc, auto func) mutable {
        if (func.name != "constructor") {
          acc[idx] = func.name;
          for (size_t i = 0; i < idx; i++) {
            if (func.name == acc[i]) {
              acc[idx] = "";
              break;
            }
          }
        } else
          acc[idx] = "";
        idx++;
        return acc;
      });

  constexpr auto overloadNum = std::apply(
      [](auto... names) {
        return (0 + ... + static_cast<size_t>(!names.empty()));
      },
      overloadNames);

  return std::tuple{funcFields.size, overloadNum};
}

template <typename T, size_t OverloadNum, size_t Index, size_t... Ns>
constexpr auto GetOverloadFuncListAt(std::index_sequence<Ns...>) {
  constexpr auto funcFields = My::MySRefl::TypeInfo<T>::DFS_Acc(
      MySRefl::ElemList<>{}, [](auto acc, auto baseType, size_t) {
        return baseType.fields.Accumulate(acc, [](auto acc, auto field) {
          if constexpr (field.is_func)
            return acc.Push(field);
          else
            return acc;
        });
      });

  constexpr auto overloadNames = funcFields.Accumulate(
      std::array<std::string_view, funcFields.size>{},
      [idx = static_cast<size_t>(0)](auto acc, auto func) mutable {
        if (func.name != "constructor") {
          acc[idx] = func.name;
          for (size_t i = 0; i < idx; i++) {
            if (func.name == acc[i]) {
              acc[idx] = "";
              break;
            }
          }
        } else
          acc[idx] = "";
        idx++;
        return acc;
      });

  constexpr auto indices = funcFields.Accumulate(
      std::array<size_t, OverloadNum>{},
      [overloadNames, idx = static_cast<size_t>(0),
       indicesCur = static_cast<size_t>(0)](auto acc, auto func) mutable {
        if (!overloadNames[idx].empty()) acc[indicesCur++] = idx;
        idx++;
        return acc;
      });

  constexpr auto name = funcFields.Get<indices[Index]>().name;

  constexpr auto masks = funcFields.Accumulate(
      std::array<bool, funcFields.size>{},
      [name, idx = static_cast<size_t>(0)](auto acc, auto func) mutable {
        acc[idx++] = func.name == name;
        return acc;
      });

  constexpr auto funcList = funcFields.template Accumulate<masks[Ns]...>(
      MySRefl::ElemList<>{},
      [](auto acc, auto func) { return acc.Push(func); });

  return funcList;
}

template <typename T, size_t... Ns, size_t... Ms>
constexpr auto GetOverloadFuncListTuple(std::index_sequence<Ns...>,
                                        std::index_sequence<Ms...>) {
  constexpr size_t OverloadNum = sizeof...(Ms);
  return std::tuple{GetOverloadFuncListAt<T, OverloadNum, Ms>(
      std::index_sequence<Ns...>{})...};
}

template <typename T>
constexpr auto GetOverload() {
  constexpr auto funcNumOverloadNum = GetFuncNumOverloadNum<T>();

  return GetOverloadFuncListTuple<T>(
      std::make_index_sequence<std::get<0>(funcNumOverloadNum)>{},
      std::make_index_sequence<std::get<1>(funcNumOverloadNum)>{});
}
}  // namespace My::MyLuaPP::detail

namespace My::MyLuaPP {
template <typename T>
void Register(lua_State* L) {
  sol::state_view lua(L);
  sol::table typeinfo = lua["MySRefl_TypeInfo"].get_or_create<sol::table>();
  detail::NameInfo nameInfo(My::MySRefl::TypeInfo<T>::name);

  sol::usertype<T> type = lua.new_usertype<T>(
      nameInfo.rawName,
      detail::GetInits<T>(
          std::make_index_sequence<My::MySRefl::TypeInfo<T>::fields.size>{}));
  MySRefl::TypeInfo<T>::DFS_ForEach([&](auto t, size_t) {
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

  constexpr auto overloadFuncListTuple = detail::GetOverload<T>();
  std::apply(
      [&](auto... funcLists) {
        (std::apply(
             [&](auto... funcs) {
               auto packedFuncs = sol::overload(funcs.value...);
               // type.set_function(std::get<0>(std::tuple{ funcs... }).name,
               // sol::overload(funcs.value...));
               auto name = std::get<0>(std::tuple{funcs...}).name;
               if (name == "operator+")
                 type[sol::meta_function::addition] = packedFuncs;
               else if (name == "operator-")
                 type[sol::meta_function::subtraction] = packedFuncs;
               else if (name == "operator*")
                 type[sol::meta_function::multiplication] = packedFuncs;
               else if (name == "operator/")
                 type[sol::meta_function::division] = packedFuncs;
               else if (name == "operator<")
                 type[sol::meta_function::less_than] = packedFuncs;
               else if (name == "operator<=")
                 type[sol::meta_function::less_than_or_equal_to] = packedFuncs;
               else if (name == "operator==")
                 type[sol::meta_function::equal_to] = packedFuncs;
               else if (name == "operator[]")
                 type[sol::meta_function::index] = packedFuncs;
               else if (name == "operator()")
                 type[sol::meta_function::call] = packedFuncs;
               else
                 type[name] = packedFuncs;
               constexpr bool needPostfix = sizeof...(funcs) > 0;
               MySRefl::ElemList{funcs...}.ForEach(
                   [&, idx = static_cast<size_t>(0)](auto func) mutable {
                     std::string name =
                         std::string(func.name) +
                         (needPostfix ? ("_" + std::to_string(idx)) : "");
                     sol::table typeinfo_type_fields_field =
                         typeinfo_type_fields[name].get_or_create<sol::table>();
                     sol::table typeinfo_type_fields_field_attrs =
                         typeinfo_type_fields_field["attrs"]
                             .get_or_create<sol::table>();
                     if constexpr (func.attrs.size > 0) {
                       func.attrs.ForEach([&](auto attr) {
                         if constexpr (attr.has_value)
                           typeinfo_type_fields_field_attrs[attr.name] =
                               attr.value;
                         else
                           typeinfo_type_fields_field_attrs[attr.name] =
                               true;  // default
                       });
                     }
                     idx++;
                   });
             },
             funcLists.elems),
         ...);
      },
      overloadFuncListTuple);

  // variable
  MySRefl::TypeInfo<T>::DFS_ForEach([&](auto t, size_t) {
    t.fields.ForEach([&](auto field) {
      if constexpr (!field.is_func) {
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
      }
    });
  });
}
}  // namespace My::MyLuaPP
