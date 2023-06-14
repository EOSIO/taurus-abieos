#pragma once
#include "check.hpp"
#include <string_view>
#include <vector>

#ifdef ABIEOS_HAS_PROTOBUF

// clang-format off
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/util/json_util.h>
// clang-format on

namespace eosio {
namespace gpb = google::protobuf;

namespace protobuf {
   class message_converter {
      gpb::DescriptorPool pool;
      const gpb::Message* get_proto_message(const std::string& message_name);
    public:
      message_converter(const gpb::FileDescriptorSet& descriptors);
      std::string to_json(const std::string& message_name, std::string_view serialized_message,
                          const gpb::util::JsonPrintOptions& options);
      std::string to_json(const std::string& message_name, std::string_view serialized_message) {
         gpb::util::JsonPrintOptions default_print_options;
         default_print_options.preserve_proto_field_names = true;
         return to_json(message_name, serialized_message, default_print_options);
      }
      std::vector<char> from_json(const std::string& message_name, std::string_view message_json,
                                  const gpb::util::JsonParseOptions& options = gpb::util::JsonParseOptions{});
   };

   bool schema_to_descriptor_set(const char* file, gpb::FileDescriptorSet& fds);
} // namespace protobuf
} // namespace eosio

#else

namespace google::protobuf {
class FileDescriptorSet {
 public:
   int file_size() const { return 0; }
};

struct Message {};

} // namespace google::protobuf

namespace eosio {
namespace gpb = google::protobuf;
namespace protobuf {

struct message_converter {
   message_converter(const gpb::FileDescriptorSet& descriptors) {}

   static std::string to_json(const gpb::Message& message) { return {}; }
   std::string        to_json(const std::string& message_name, std::string_view serialized_message) {
             return {};
   }
   std::vector<char> from_json(const std::string& message_name, std::string_view message_json) { return {}; }
};
}
} // namespace eosio

#endif
