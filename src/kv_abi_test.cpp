
#include <eosio/abi.hpp>
#include <eosio/to_key.hpp>

int error_count = 0;

void report_error(const char* assertion, const char* file, int line) {
    if (error_count <= 20) {
        std::printf("%s:%d: failed %s\n", file, line, assertion);
    }
    ++error_count;
}

#define CHECK(...)                                                                                                     \
    do {                                                                                                               \
        if (__VA_ARGS__) {                                                                                             \
        } else {                                                                                                       \
            report_error(#__VA_ARGS__, __FILE__, __LINE__);                                                            \
        }                                                                                                              \
    } while (0)



const char* kv_abi = R"(
{
    "____comment": "This file was generated with eosio-abigen. DO NOT EDIT ",
    "version": "eosio::abi/1.3",
    "types": [],
    "structs": [
        {
            "name": "my_struct",
            "base": "",
            "fields": [
                {
                    "name": "primary_key",
                    "type": "name"
                },
                {
                    "name": "note",
                    "type": "string"
                }
            ]
        },
        {
            "name": "log_config",
            "base": "",
            "fields": [
                {
                    "name": "log_level",
                    "type": "uint8"
                }
            ]
        }
    ],
    "actions": [],
    "tables": [],
    "ricardian_clauses": [],
    "variants": [],
    "kv_tables": {
        "testtable": {
            "type": "my_struct",
            "primary_index": {
                "name": "primary",
                "type": "name"
            },
            "secondary_indices": {
                "note": {
                    "type": "string"
                }
            }
        },
        "logconf": {
            "type": "log_config",
            "primary_index": {},
            "secondary_indices": {}
        }
    }
}
)";

int main(int argc, const char** argv) {

    using namespace eosio::literals;
    using namespace std::literals;

    eosio::abi abi{kv_abi};

    { // test regular kv table primary key

        auto key = eosio::convert_to_key(std::make_tuple(uint8_t(1), "testtable"_n, "primary"_n, "test"s));
        std::vector<char> buffer;
        eosio::vector_stream strm(buffer);
        to_bin(std::make_tuple("taurus"_n,"note"s), strm);


        auto result = abi.kv_table_primary_index_to_json(eosio::input_stream(key), eosio::input_stream(buffer));
        CHECK(std::string_view{R"({"primary_key":"taurus","note":"note"})"} == result);
    }
    { // test regular kv table secondary key

        auto key = eosio::convert_to_key(std::make_tuple(uint8_t(1), "testtable"_n, "note"_n, "note"s));
        std::vector<char> buffer;
        eosio::vector_stream strm(buffer);
        to_bin(std::make_tuple("taurus"_n,"note"s), strm);


        auto result = abi.kv_table_primary_index_to_json(eosio::input_stream(key), eosio::input_stream(buffer));
        CHECK(result.empty());
    }
    return error_count;
}
