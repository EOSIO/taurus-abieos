#pragma once
#include "name.hpp"
#include "types.hpp"
#include <functional>
#include <map>
#include <string>
#include <variant>
#include <vector>
#include "fixed_bytes.hpp"
#include "crypto.hpp"
#include "varint.hpp"
#include "float.hpp"
#include "time.hpp"
#include "bytes.hpp"
#include "asset.hpp"
#include "opaque.hpp"
#include "pb_support.hpp"

namespace eosio {

enum class abi_error {
   no_error,

   recursion_limit_reached,
   invalid_nesting,
   unknown_type,
   missing_name,
   redefined_type,
   base_not_a_struct,
   extension_typedef,
   bad_abi
};

constexpr inline std::string_view convert_abi_error(eosio::abi_error e) {
   switch (e) {
      case abi_error::no_error: return "No error";
      case abi_error::recursion_limit_reached: return "Recursion limit reached";
      case abi_error::invalid_nesting: return "Invalid nesting";
      case abi_error::unknown_type: return "Unknown type";
      case abi_error::missing_name: return "Missing name";
      case abi_error::redefined_type: return "Redefined type";
      case abi_error::base_not_a_struct: return "Base not a struct";
      case abi_error::extension_typedef: return "Extension typedef";
      case abi_error::bad_abi: return "Bad ABI";
      default: return "internal failure";
   };
}

struct abi_serializer;

template <typename T>
struct might_not_exist {
   T value{};
};

template <typename T, typename S>
void from_bin(might_not_exist<T>& obj, S& stream) {
   if (stream.remaining())
      return from_bin(obj.value, stream);
}

template <typename T, typename S>
void to_bin(const might_not_exist<T>& obj, S& stream) {
   return to_bin(obj.value, stream);
}

template <typename T, typename S>
void from_json(might_not_exist<T>& obj, S& stream) {
   return from_json(obj.value, stream);
}

template <typename T, typename S>
void to_json(const might_not_exist<T>& val, S& stream) {
   return to_json(val.value, stream);
}

using abi_extensions_type = std::vector<std::pair<uint16_t, std::vector<char>>>;

struct type_def {
   std::string new_type_name{};
   std::string type{};
};

EOSIO_REFLECT(type_def, new_type_name, type);

struct field_def {
   std::string name{};
   std::string type{};
};

EOSIO_REFLECT(field_def, name, type);

struct struct_def {
   std::string            name{};
   std::string            base{};
   std::vector<field_def> fields{};
};

EOSIO_REFLECT(struct_def, name, base, fields);

struct action_def {
   eosio::name name{};
   std::string type{};
   std::string ricardian_contract{};
};

EOSIO_REFLECT(action_def, name, type, ricardian_contract);

struct table_def {
   eosio::name              name{};
   std::string              index_type{};
   std::vector<std::string> key_names{};
   std::vector<std::string> key_types{};
   std::string              type{};
};

EOSIO_REFLECT(table_def, name, index_type, key_names, key_types, type);

struct clause_pair {
   std::string id{};
   std::string body{};
};

EOSIO_REFLECT(clause_pair, id, body);

struct error_message {
   uint64_t    error_code{};
   std::string error_msg{};
};

EOSIO_REFLECT(error_message, error_code, error_msg);

struct variant_def {
   std::string              name{};
   std::vector<std::string> types{};
};

EOSIO_REFLECT(variant_def, name, types);

struct action_result_def {
   eosio::name name{};
   std::string result_type{};
};

EOSIO_REFLECT(action_result_def, name, result_type);

struct primary_key_index_def {
   eosio::name name{};
   std::string type;
};

EOSIO_REFLECT(primary_key_index_def, name, type);

struct secondary_index_def {
   std::string type;
};

EOSIO_REFLECT(secondary_index_def, type);

struct kv_table_entry_def {
   std::string                                type;
   primary_key_index_def                      primary_index;
   std::map<eosio::name, secondary_index_def> secondary_indices;
};

EOSIO_REFLECT(kv_table_entry_def, type, primary_index, secondary_indices);

struct abi_def {
   std::string                                                version{};
   std::vector<type_def>                                      types{};
   std::vector<struct_def>                                    structs{};
   std::vector<action_def>                                    actions{};
   std::vector<table_def>                                     tables{};
   std::vector<clause_pair>                                   ricardian_clauses{};
   std::vector<error_message>                                 error_messages{};
   abi_extensions_type                                        abi_extensions{};
   might_not_exist<std::vector<variant_def>>                  variants{};
   might_not_exist<std::vector<action_result_def>>            action_results{};
   might_not_exist<std::map<eosio::name, kv_table_entry_def>> kv_tables{};
   might_not_exist<gpb::FileDescriptorSet>                    protobuf_types;

   static std::vector<char> json_to_bin(std::string json);
   static std::string bin_to_json(eosio::input_stream bin);
};

EOSIO_REFLECT(abi_def, version, types, structs, actions, tables, ricardian_clauses, error_messages, abi_extensions,
              variants, action_results, kv_tables, protobuf_types);

struct abi_type;

struct abi_field {
   std::string     name;
   const abi_type* type;
};

struct abi_type {
   std::string name;

   struct builtin {};
   using alias_def = std::string;
   struct alias {
      abi_type* type;
   };
   struct optional {
      abi_type* type;
   };
   struct extension {
      abi_type* type;
   };
   struct array {
      abi_type* type;
   };
   struct szarray {
      abi_type* type;
      std::size_t size;
   };
   struct struct_ {
      abi_type*              base = nullptr;
      std::vector<abi_field> fields;
   };
   using variant = std::vector<abi_field>;
   std::variant<builtin, const alias_def*, const struct_def*, const variant_def*, alias, optional, extension, array, szarray,
                struct_, variant>
                         _data;
   const abi_serializer* ser = nullptr;

   template <typename T>
   abi_type(std::string name, T&& arg, const abi_serializer* ser)
       : name(std::move(name)), _data(std::forward<T>(arg)), ser(ser) {}
   abi_type(const abi_type&) = delete;
   abi_type& operator=(const abi_type&) = delete;

   // result<void> json_to_bin(std::vector<char>& bin, std::string_view json);
   const abi_type* optional_of() const {
      if (auto* t = std::get_if<optional>(&_data))
         return t->type;
      else
         return nullptr;
   }
   const abi_type* extension_of() const {
      if (auto* t = std::get_if<extension>(&_data))
         return t->type;
      else
         return nullptr;
   }
   const abi_type* array_of() const {
      if (auto* t = std::get_if<array>(&_data))
         return t->type;
      else
         return nullptr;
   }
   const abi_type* szarray_of() const {
      if (auto* t = std::get_if<szarray>(&_data))
         return t->type;
      else
         return nullptr;
   }

   const struct_* as_struct() const { return std::get_if<struct_>(&_data); }
   const variant* as_variant() const { return std::get_if<variant>(&_data); }
   const szarray* as_szarray() const { return std::get_if<szarray>(&_data); }

   const abi_serializer* get_serializer() const { 
      const alias* a = std::get_if<alias>(&_data);
      if (a) {
         return a->type->get_serializer();
      }
      return ser;
   }

   std::vector<char> json_to_bin_reorderable(
         std::string_view json, std::function<void()> f = [] {}) const;

   std::string bin_to_json(input_stream bin) const;
   std::vector<char> json_to_bin(std::string_view json) const;
};

struct abi {
   std::map<eosio::name, std::string> action_types;
   std::map<eosio::name, std::string> table_types;
   std::map<eosio::name, std::string> kv_tables;
   std::map<eosio::name, std::string> kv_table_types;
   std::map<eosio::name, eosio::name> kv_table_primary_key_name;
   std::map<std::string, abi_type>    abi_types;
   std::map<eosio::name, std::string> action_result_types;
   const abi_type*                    get_type(const std::string& name);
   std::unique_ptr<eosio::protobuf::message_converter> protobuf_converter;

   // Adds a type to the abi.  Has no effect if the type is already present.
   // If the type is a struct, all members will be added recursively.
   // Exception Safety: basic. If add_type fails, some objects may have
   // an incomplete list of fields.
   template <typename T>
   abi_type* add_type();

   std::string convert_to_json(const char* type, input_stream bin);
   std::vector<char> convert_to_bin(const char* type,  std::string_view json);

   abi() = default;
   abi(abi&&) noexcept = default;
   // constrcut an abi object from json
   abi(std::string json_string);


   // construct an abi object from binary format
   abi(input_stream bin);
   abi(const std::vector<char>& bin) : abi(input_stream{bin.data(), bin.size()}) {}
   abi& operator=(abi&&) noexcept = default;


   std::string action_bin_to_json(eosio::name action, input_stream bin) {
      auto itr = action_types.find(action);
      if (itr != action_types.end())
         return convert_to_json(itr->second.c_str(), bin);
      return {};
   }

   std::vector<char> action_json_to_bin(eosio::name action, std::string_view json){
      auto itr = action_types.find(action);
      if (itr != action_types.end())
         return convert_to_bin(itr->second.c_str(), json);
      return {};
   }

   std::string action_result_bin_to_json(eosio::name action, input_stream bin){
      auto itr = action_result_types.find(action);
      if (itr != action_result_types.end())
         return convert_to_json(itr->second.c_str(), bin);
      return {};
   }

   std::vector<char> action_result_json_to_bin(eosio::name action, std::string_view json){
      auto itr = action_result_types.find(action);
      if (itr != action_result_types.end())
         return convert_to_bin(itr->second.c_str(), json);
      return {};
   }

   std::string table_bin_to_json(eosio::name tablename, input_stream bin){
      auto itr = table_types.find(tablename);
      if (itr != table_types.end())
         return convert_to_json(itr->second.c_str(), bin);
      return {};
   }

   std::string kv_table_bin_to_json(eosio::name tablename, input_stream bin){
      auto itr = kv_table_types.find(tablename);
      if (itr != kv_table_types.end())
         return convert_to_json(itr->second.c_str(), bin);
      return {};
   }

   std::string kv_table_primary_index_to_json(input_stream key, input_stream value);
};

void convert(const abi_def& def, abi&);
void convert(const abi& def, abi_def&);

extern const abi_serializer* const object_abi_serializer;
extern const abi_serializer* const variant_abi_serializer;
extern const abi_serializer* const array_abi_serializer;
extern const abi_serializer* const szarray_abi_serializer;
extern const abi_serializer* const szbytes_abi_serializer;
extern const abi_serializer* const extension_abi_serializer;
extern const abi_serializer* const optional_abi_serializer;

using basic_abi_types =
      std::tuple<bool, int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t, int64_t, uint64_t, __int128, unsigned __int128,
               varuint32, varint32, float, double, float128, time_point, time_point_sec, block_timestamp, name,
               bytes, std::string, checksum160, checksum256, checksum512, public_key, private_key, signature, 
               symbol, symbol_code, asset>;

namespace detail {
   template <typename U, typename... T>
   constexpr bool contains(std::tuple<T...>*) {
      return (std::is_same_v<U, T> || ...);
   }
} // namespace detail

template <typename T>
constexpr bool is_basic_abi_type(T*) {
   return detail::contains<T>((basic_abi_types*)nullptr);
}

template <typename T>
constexpr bool is_basic_abi_type_v = is_basic_abi_type((T*)nullptr);

template <typename T>
auto add_type(abi& a, T*) -> std::enable_if_t<reflection::has_for_each_field_v<T> && !is_basic_abi_type_v<T>, abi_type*> {
   std::string name      = get_type_name((T*)nullptr);
   auto [iter, inserted] = a.abi_types.try_emplace(name, name, abi_type::struct_{}, object_abi_serializer);
   if (!inserted)
      return &iter->second;
   auto& s = std::get<abi_type::struct_>(iter->second._data);
   for_each_field<T>([&](const char* name, auto&& member) {
      auto member_type = a.add_type<std::decay_t<decltype(member((T*)nullptr))>>();
      s.fields.push_back({ name, member_type });
   });
   return &iter->second;
}

template <typename T>
auto add_type(abi& a, T* t) -> std::enable_if_t<is_basic_abi_type_v<T>, abi_type*> {
   std::string name = get_type_name(t);
   auto iter = a.abi_types.find(name);
   check(iter != a.abi_types.end(), "Unknown type: " + name);
   return &iter->second;
}

template <typename T>
abi_type* add_type(abi& a, std::vector<T>*) {
   auto element_type = a.add_type<T>();
   std::string name      = get_type_name((std::vector<T>*)nullptr);
   check(!(element_type->optional_of() || element_type->array_of() || element_type->extension_of()),
         "Invalid nesting: " + name);
   auto [iter, inserted] = a.abi_types.try_emplace(name, name, abi_type::array{ element_type }, array_abi_serializer);
   return &iter->second;
}

template <std::size_t SZ>
abi_type* add_type(abi& a, std::array<uint8_t, SZ>*) {
   auto element_type = a.add_type<uint8_t>();
   std::string name      = get_type_name((std::array<uint8_t, SZ>*)nullptr);
   auto [iter, inserted] = a.abi_types.try_emplace(name, name, abi_type::szarray{ element_type, SZ}, szbytes_abi_serializer);
   return &iter->second;
}

template <std::size_t SZ>
abi_type* add_type(abi& a, std::array<char, SZ>*) {
   auto element_type = a.add_type<char>();
   std::string name      = get_type_name((std::array<int8_t, SZ>*)nullptr);
   auto [iter, inserted] = a.abi_types.try_emplace(name, name, abi_type::szarray{ element_type, SZ}, szbytes_abi_serializer);
   return &iter->second;
}

template <typename T, std::size_t SZ>
abi_type* add_type(abi& a, std::array<T, SZ>*) {
   auto element_type = a.add_type<T>();
   std::string name      = get_type_name((std::array<T, SZ>*)nullptr);
   check(!(element_type->optional_of() || element_type->array_of() || element_type->extension_of()),
         "Invalid nesting: " + name);
   auto [iter, inserted] = a.abi_types.try_emplace(name, name, abi_type::szarray{ element_type , SZ}, szarray_abi_serializer);
   return &iter->second;
}

template <typename... T>
auto add_type(abi& a, std::variant<T...>*) -> std::enable_if_t<!is_basic_abi_type_v<std::variant<T...>>, abi_type*> {
   abi_type::variant types;
   (
         [&](auto* t) {
            auto type = add_type(a, t);
            types.push_back({ type->name, type });
         }((T*)nullptr),
         ...);
   std::string name = get_type_name((std::variant<T...>*)nullptr);

   auto [iter, inserted] = a.abi_types.try_emplace(name, name, std::move(types), variant_abi_serializer);
   return &iter->second;
}

template <typename T>
abi_type* add_type(abi& a, std::optional<T>*) {
   auto element_type = a.add_type<T>();
   std::string name = get_type_name((std::optional<T>*)nullptr);
   check(!(element_type->optional_of() || element_type->array_of() || element_type->extension_of()),
         "Invalid nesting: " + name);
   auto [iter, inserted] =
         a.abi_types.try_emplace(name, name, abi_type::optional{ element_type }, optional_abi_serializer);
   return &iter->second;
}

template <typename T>
abi_type* add_type(abi& a, opaque<T>*) {
   a.add_type<T>();
   auto iter = a.abi_types.find("bytes");
   check(iter != a.abi_types.end(), convert_abi_error(abi_error::unknown_type));
   return &iter->second;
}

template <typename T>
abi_type* add_type(abi& a, might_not_exist<T>*) {
   auto element_type = a.add_type<T>();
   std::string name = element_type->name + "$";
   check(!element_type->extension_of(), "Invalid nesting: " + name);
   auto [iter, inserted] =
         a.abi_types.try_emplace(name, name, abi_type::extension{ element_type }, extension_abi_serializer);
   return &iter->second;
}

template <typename T>
abi_type* abi::add_type() {
   using eosio::add_type;
   return add_type(*this, (T*)nullptr);
}

template <typename T, typename S>
void to_json_write_helper(const T& field, const std::string_view field_name, const bool need_comma, S& stream) {
   if (need_comma) {
      stream.write(',');
   }
   to_json(field_name, stream);
   stream.write(':');
   to_json(field, stream);
}

template <typename S>
void to_json(const abi_def& def, S& stream) {
   stream.write('{');
   to_json_write_helper(def.version, "version", false, stream);
   to_json_write_helper(def.types, "types", true, stream);
   to_json_write_helper(def.structs, "structs", true, stream);
   to_json_write_helper(def.actions, "actions", true, stream);
   to_json_write_helper(def.tables, "tables", true, stream);
   to_json_write_helper(def.ricardian_clauses, "ricardian_clauses", true, stream);
   to_json_write_helper(def.error_messages, "error_messages", true, stream);
   to_json_write_helper(def.variants.value, "variants", true, stream);
   to_json_write_helper(def.action_results.value, "action_results", true, stream);
   to_json_write_helper(def.kv_tables.value, "kv_tables", true, stream);
   if (def.protobuf_types.value.file_size())
      to_json_write_helper(def.protobuf_types, "protobuf_types", true, stream);
   stream.write('}');
}

} // namespace eosio
