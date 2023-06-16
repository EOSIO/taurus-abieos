#include <eosio/pb_support.hpp>
#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/dynamic_message.h>
#include <stdlib.h>
#include <limits.h>
#include <cstring>

namespace gpbc = google::protobuf::compiler;

namespace eosio::protobuf {

message_converter::message_converter(const google::protobuf::FileDescriptorSet& descriptors) {
    for (int i = 0; i < descriptors.file_size(); ++i) {
        auto file_desc_proto = descriptors.file(i);
        check(pool.BuildFile(file_desc_proto) != NULL, file_desc_proto.DebugString());
    }
}

const gpb::Message* message_converter::get_proto_message(const std::string& message_name) {
    const gpb::Descriptor* message_desc = pool.FindMessageTypeByName(message_name);
    check(message_desc, "message not found");
    static gpb::DynamicMessageFactory factory;
    const gpb::Message* prototype_msg = factory.GetPrototype(message_desc); // prototype_msg is immutable
    check(prototype_msg, "cannot create prototype message from message descriptor");
    return prototype_msg;
}

std::string message_converter::to_json(const std::string& message_name, std::string_view serialized_message,
                                       const gpb::util::JsonPrintOptions& options) {
    std::unique_ptr<gpb::Message> mutable_msg(get_proto_message(message_name)->New());
    check(mutable_msg->ParseFromArray(serialized_message.data(), serialized_message.size()),
          "failed to parse value in serialized_message");
    std::string result;
    auto status = gpb::util::MessageToJsonString(*mutable_msg, &result, options);
    check(status.ok(), status.message().as_string());
    return result;
}

std::vector<char> message_converter::from_json(const std::string& message_name, std::string_view message_json,
                                               const gpb::util::JsonParseOptions& options) {
    std::unique_ptr<gpb::Message> mutable_msg(get_proto_message(message_name)->New());
    auto status = gpb::util::JsonStringToMessage(gpb::StringPiece(message_json.data(), message_json.size()),
                                                 mutable_msg.get(), options);
    check(status.ok(), status.message().as_string());
    std::vector<char> result(mutable_msg->ByteSizeLong());
    check(mutable_msg->SerializeToArray(result.data(), result.size()), "unable to serialize message");
    return result;
}

class ErrorCollector : public gpbc::MultiFileErrorCollector {
  public:
    ErrorCollector() {}
    ~ErrorCollector() {}

    // implements ErrorCollector ---------------------------------------
    void AddError(const std::string& filename, int line, int column, const std::string& message) override {
        std::cerr << "error filename " << filename << " (" << line << ", " << column << ")  message " << message
                  << std::endl;
    }
};

bool schema_to_descriptor_set(const char* file, gpb::FileDescriptorSet& fds) {
    char the_proto_file[PATH_MAX];
    char* absolute_filename = realpath(file, the_proto_file);
    if (absolute_filename == nullptr)
        return false;

    char* last_slash_pos = std::strrchr(absolute_filename, '/');
    *last_slash_pos = '\0';
    const char* filename = last_slash_pos+1;
    
    gpbc::DiskSourceTree source_tree;
    source_tree.MapPath("", absolute_filename);

    ErrorCollector collector;

    gpbc::SourceTreeDescriptorDatabase db(&source_tree);
    db.RecordErrorsTo(&collector);

    gpb::FileDescriptorProto desc;
    return db.FindFileByName(filename, fds.add_file());
}
} // namespace eosio::protobuf