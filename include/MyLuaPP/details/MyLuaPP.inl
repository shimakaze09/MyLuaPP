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
    size_t cur = name[0] == 's'   ? 6   // struct
                 : name[0] == 'c' ? 5   // class
                                  : 4;  // enum
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
  constexpr auto masks = MySRefl::TypeInfo<T>::fields.Accumulate(
      std::array<bool, MySRefl::TypeInfo<T>::fields.size>{},
      [idx = 0](auto&& acc, auto field) mutable {
        acc[idx++] = field.name == MySRefl::Name::constructor;
        return std::forward<decltype(acc)>(acc);
      });
  constexpr auto constructors =
      MySRefl::TypeInfo<T>::fields.template Accumulate<masks[Ns]...>(
          std::tuple<>{}, [](auto acc, auto field) {
            return std::tuple_cat(acc, std::tuple{field.value});
          });
  if constexpr (std::tuple_size_v<std::decay_t<decltype(constructors)>> > 0)
    return std::apply([](auto... elems) { return sol::initializers(elems...); },
                      constructors);
  else if constexpr (std::is_default_constructible_v<T>)
    return sol::initializers(MySRefl::WrapConstructor<T()>());
  else
    return sol::no_constructor;
}

// sizeof...(Ns) is the field number
template <typename T, typename Acc, size_t... Ns, size_t... Indices>
constexpr auto GetOverloadFuncListTupleRec(Acc acc, std::index_sequence<Ns...>,
                                           std::index_sequence<Indices...>) {
  if constexpr (sizeof...(Indices) > 0) {
    using IST =
        MySRefl::detail::IntegerSequenceTraits<std::index_sequence<Indices...>>;
    if constexpr (IST::head != static_cast<size_t>(-1)) {
      constexpr auto masks = MySRefl::TypeInfo<T>::fields.Accumulate(
          std::array<bool, MySRefl::TypeInfo<T>::fields.size>{},
          [idx = static_cast<size_t>(0)](auto acc, auto field) mutable {
            acc[idx++] =
                field.name ==
                MySRefl::TypeInfo<T>::fields.template Get<IST::head>().name;
            return acc;
          });
      constexpr auto funclist =
          MySRefl::TypeInfo<T>::fields.template Accumulate<masks[Ns]...>(
              MySRefl::ElemList<>{},
              [](auto acc, auto func) { return acc.Push(func); });
      return GetOverloadFuncListTupleRec<T>(
          std::tuple_cat(acc, std::tuple{funclist}),
          std::index_sequence<Ns...>{}, IST::tail);
    } else
      return GetOverloadFuncListTupleRec<T>(acc, std::index_sequence<Ns...>{},
                                            IST::tail);
  } else
    return acc;
}

// sizeof...(Ns) is the field number
template <typename T, size_t... Ns>
constexpr auto GetOverloadFuncListTupleImpl(std::index_sequence<Ns...>) {
  if constexpr (MySRefl::TypeInfo<T>::bases.size == 0) {
    constexpr auto names =
        std::array{MySRefl::TypeInfo<T>::fields.template Get<Ns>().name...};

    constexpr auto indices = MySRefl::TypeInfo<T>::fields.Accumulate(
        std::array<size_t, MySRefl::TypeInfo<T>::fields.size>{},
        [names, idx = static_cast<size_t>(0)](auto acc, auto field) mutable {
          if constexpr (field.is_func) {
            acc[idx] = idx;
            for (size_t i = 0; i < idx; i++) {
              if (field.name == names[i] ||
                  field.name == MySRefl::Name::constructor ||
                  field.name == MySRefl::Name::destructor) {
                acc[idx] = static_cast<size_t>(-1);
                break;
              }
            }
          } else
            acc[idx] = static_cast<size_t>(-1);

          idx++;
          return acc;
        });
    return GetOverloadFuncListTupleRec<T>(
        std::tuple<>{}, std::index_sequence<Ns...>{},
        std::index_sequence<indices[Ns]...>{});
  } else
    static_assert(false, "DFS");
}

template <typename T>
constexpr auto GetOverloadFuncListTuple() {
  return GetOverloadFuncListTupleImpl<T>(
      std::make_index_sequence<MySRefl::TypeInfo<T>::fields.size>());
}

template <typename T>
void RegisterClass(lua_State* L) {
  sol::state_view lua(L);
  sol::table typeinfo = lua["MySRefl_TypeInfo"].get_or_create<sol::table>();
  NameInfo nameInfo(MySRefl::TypeInfo<T>::name);

  sol::usertype<T> type = lua.new_usertype<T>(
      nameInfo.rawName,
      GetInits<T>(
          std::make_index_sequence<MySRefl::TypeInfo<T>::fields.size>{}));

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

  constexpr auto overloadFuncListTuple = GetOverloadFuncListTuple<T>();
  std::apply(
      [&](auto... funcLists) {
        (std::apply(
             [&](auto... funcs) {
               auto packedFuncs = sol::overload(funcs.value...);
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
        type[field.name] = field.value;

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

template <typename T>
void RegisterEnum(lua_State* L) {
  sol::state_view lua(L);
  NameInfo nameInfo(MySRefl::TypeInfo<T>::name);
  sol::table typeinfo = lua["MySRefl_TypeInfo"].get_or_create<sol::table>();
  sol::table typeinfo_enum =
      typeinfo[nameInfo.rawName].get_or_create<sol::table>();
  sol::table typeinfo_enum_attrs =
      typeinfo_enum["attrs"].get_or_create<sol::table>();
  sol::table typeinfo_enum_fields =
      typeinfo_enum["fields"].get_or_create<sol::table>();

  MySRefl::TypeInfo<T>::attrs.ForEach([&](auto attr) {
    if constexpr (attr.has_value)
      typeinfo_enum_attrs[attr.name] = attr.value;
    else
      typeinfo_enum_attrs[attr.name] = true;  // default
  });

  MySRefl::TypeInfo<T>::fields.ForEach([&](auto field) {
    sol::table typeinfo_enum_fields_field =
        typeinfo_enum_fields[field.name].get_or_create<sol::table>();
    sol::table typeinfo_type_fields_field_attrs =
        typeinfo_enum_fields_field["attrs"].get_or_create<sol::table>();
    field.attrs.ForEach([&](auto attr) {
      if constexpr (attr.has_value)
        typeinfo_type_fields_field_attrs[attr.name] = attr.value;
      else
        typeinfo_type_fields_field_attrs[attr.name] = true;  // default
    });
  });

  constexpr auto nvs = MySRefl::TypeInfo<T>::fields.Accumulate(
      std::tuple<>{}, [](auto acc, auto field) {
        return std::tuple_cat(acc, std::tuple{field.name, field.value});
      });
  std::apply([&](auto... values) { lua.new_enum(nameInfo.rawName, values...); },
             nvs);
}
}  // namespace My::MyLuaPP::detail

namespace My::MyLuaPP {
template <typename T>
void Register(lua_State* L) {
  if constexpr (std::is_enum_v<T>)
    detail::RegisterEnum<T>(L);
  else
    detail::RegisterClass<T>(L);
}
}  // namespace My::MyLuaPP
