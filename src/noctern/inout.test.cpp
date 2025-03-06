#include "./inout.hpp"

#include <catch2/catch.hpp>

namespace noctern {
    namespace {
        struct point {
            int x;
            int y;
        };

        TEST_CASE("in works") {
            int x = 42;
            in<int> param = in(x);

            CHECK(*param == 42);

            SECTION("->") {
                point x {1, 2};
                in<point> param = in(x);
                CHECK(param->x == 1);
            }
        }

        TEST_CASE("out works") {
            int x = 42;
            out<int> param = out(x);
            *param = 45;

            CHECK(x == 45);

            SECTION("->") {
                point x {1, 2};
                out<point> param = out(x);
                CHECK(param->x == 1);
            }
        }

        TEST_CASE("inout works") {
            int x = 42;
            inout<int> param = inout(x);
            CHECK(*param == 42);
            *param = 45;

            CHECK(x == 45);

            SECTION("->") {
                point x {1, 2};
                inout<point> param = inout(x);
                CHECK(param->x == 1);
            }
        }
    }
}