#include <eosio/reflection.hpp>
#include <eosio/for_each_field.hpp>
#include <cstdio>
#include <string_view>

int error_count;

void report_error(const char* assertion, const char* file, int line) {
    if(error_count <= 20) {
       std::printf("%s:%d: failed %s\n", file, line, assertion);
    }
    ++error_count;
}

#define CHECK(...) do { if(__VA_ARGS__) {} else { report_error(#__VA_ARGS__, __FILE__, __LINE__); } } while(0)

struct fn {
   int test(int i) { return i * 2; }
};
EOSIO_REFLECT(fn, test);

struct outer {
   struct inner {
      int i;
      EOSIO_FRIEND_REFLECT(outer::inner, i);
   };
   inner i;
   int j, k;
   EOSIO_FRIEND_REFLECT(outer, i, j, k);
};

int main() {
   int counter = 0;
   eosio::for_each_field<fn>([&](const char* name, auto method) { ++counter; });
   CHECK(counter == 0);
   eosio::for_each_method<fn>([&](const char* name, int (fn::*m)(int)) { CHECK(m == &fn::test); ++counter; });
   CHECK(counter == 1);

   CHECK( std::string_view{get_type_name(static_cast<outer::inner*>(nullptr))} == std::string_view{"outer::inner"}); 
   counter = 0;
   eosio::for_each_field<outer::inner>([&counter](const char* name, auto method) { ++counter; });
   CHECK(counter == 1);
   counter = 0;
   eosio::for_each_field<outer>([&counter](const char* name, auto method) { ++counter; });
   CHECK(counter == 3);
   return error_count;
}
