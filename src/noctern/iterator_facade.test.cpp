#include "./iterator_facade.hpp"

#include <catch2/catch.hpp>

namespace noctern {
    namespace {
        class one_shot_ints : public iterator_facade<one_shot_ints> {
        public:
            using iterator_category = std::input_iterator_tag;
            using difference_type = int;

            explicit constexpr one_shot_ints(int n)
                : value_(n) {
            }

            constexpr int read() const {
                return value_;
            }

            constexpr void advance(val_t<1>) {
                ++value_;
            }

            constexpr bool equal_to(one_shot_ints rhs) const {
                return value_ == rhs.value_;
            }

        private:
            int value_;
        };
        static_assert(std::input_iterator<one_shot_ints>);

        class forward_ints : public iterator_facade<forward_ints> {
        public:
            constexpr forward_ints() = default;
            explicit constexpr forward_ints(int n)
                : value_(n) {
            }

            constexpr int read() const {
                return value_;
            }

            constexpr void advance(val_t<1>) {
                ++value_;
            }

            constexpr bool equal_to(forward_ints rhs) const {
                return value_ == rhs.value_;
            }

        private:
            int value_;
        };
        static_assert(std::forward_iterator<forward_ints>);

        class bidir_ints : public iterator_facade<bidir_ints> {
        public:
            constexpr bidir_ints() = default;
            explicit constexpr bidir_ints(int n)
                : value_(n) {
            }

            constexpr int read() const {
                return value_;
            }

            template <auto N>
                requires(N == 1 || N == -1)
            constexpr void advance(val_t<N>) {
                value_ += N;
            }

            constexpr bool equal_to(bidir_ints rhs) const {
                return value_ == rhs.value_;
            }

        private:
            int value_;
        };
        static_assert(std::bidirectional_iterator<bidir_ints>);

        class rand_ints : public iterator_facade<rand_ints> {
        public:
            using difference_type = int;

            constexpr rand_ints() = default;
            explicit constexpr rand_ints(int n)
                : value_(n) {
            }

            constexpr int read() const {
                return value_;
            }

            constexpr void advance(int offset) {
                value_ += offset;
            }

            constexpr int distance(rand_ints rhs) const {
                return rhs.value_ - value_;
            }

        private:
            int value_;
        };
        static_assert(std::random_access_iterator<rand_ints>);

        template <typename Iter>
        using traits = std::iterator_traits<Iter>;

        TEMPLATE_TEST_CASE("iterator_facade increment", "[motion]", one_shot_ints, forward_ints,
            bidir_ints, rand_ints) {
            TestType x(1);
            TestType& r = ++x;
            CHECK(&r == &x);
            CHECK(*x == 2);
            TestType cpy = x++;
            CHECK(*cpy == 2);
            CHECK(*x == 3);
        }

        TEMPLATE_TEST_CASE("iterator_facade decrement", "[motion]", bidir_ints, rand_ints) {
            TestType x(1);
            TestType& r = --x;
            CHECK(&r == &x);
            CHECK(*x == 0);
            TestType cpy = x--;
            CHECK(*cpy == 0);
            CHECK(*x == -1);
        }

        TEST_CASE("iterator_facade comparisons") {
            SECTION("equality") {
                CHECK(one_shot_ints(1) == one_shot_ints(1));
                CHECK(one_shot_ints(1) != one_shot_ints(2));
                CHECK(forward_ints(1) == forward_ints(1));
                CHECK(forward_ints(1) != forward_ints(2));
                CHECK(bidir_ints(1) == bidir_ints(1));
                CHECK(bidir_ints(1) != bidir_ints(2));
                CHECK(rand_ints(1) == rand_ints(1));
                CHECK(rand_ints(1) != rand_ints(2));
            }
            SECTION("less than") {
                CHECK_FALSE(rand_ints(1) < rand_ints(1));
                CHECK(rand_ints(1) < rand_ints(2));
            }
        }

        TEST_CASE("std::iterator_traits<iterator_facade>") {
            SECTION("reference") {
                STATIC_REQUIRE(std::same_as<traits<one_shot_ints>::reference, int>);
                STATIC_REQUIRE(std::same_as<traits<forward_ints>::reference, int>);
                STATIC_REQUIRE(std::same_as<traits<bidir_ints>::reference, int>);
                STATIC_REQUIRE(std::same_as<traits<rand_ints>::reference, int>);
            }
            SECTION("pointer") {
                STATIC_REQUIRE(std::same_as<traits<one_shot_ints>::pointer, void>);
                STATIC_REQUIRE(std::same_as<traits<forward_ints>::pointer, void>);
                STATIC_REQUIRE(std::same_as<traits<bidir_ints>::pointer, void>);
                STATIC_REQUIRE(std::same_as<traits<rand_ints>::pointer, void>);
            }
            SECTION("difference_type") {
                STATIC_REQUIRE(std::same_as<traits<one_shot_ints>::difference_type, int>);
                STATIC_REQUIRE(std::same_as<traits<forward_ints>::difference_type, std::ptrdiff_t>);
                STATIC_REQUIRE(std::same_as<traits<bidir_ints>::difference_type, std::ptrdiff_t>);
                STATIC_REQUIRE(std::same_as<traits<rand_ints>::difference_type, int>);
            }
            SECTION("value_type") {
                STATIC_REQUIRE(std::same_as<traits<one_shot_ints>::value_type, int>);
                STATIC_REQUIRE(std::same_as<traits<forward_ints>::value_type, int>);
                STATIC_REQUIRE(std::same_as<traits<bidir_ints>::value_type, int>);
                STATIC_REQUIRE(std::same_as<traits<rand_ints>::value_type, int>);
            }
            SECTION("iterator_category") {
                STATIC_REQUIRE(std::same_as<traits<one_shot_ints>::iterator_category,
                    std::input_iterator_tag>);
                STATIC_REQUIRE(
                    std::same_as<traits<forward_ints>::iterator_category, std::input_iterator_tag>);
                STATIC_REQUIRE(
                    std::same_as<traits<bidir_ints>::iterator_category, std::input_iterator_tag>);
                STATIC_REQUIRE(
                    std::same_as<traits<rand_ints>::iterator_category, std::input_iterator_tag>);
            }
            SECTION("iterator_concept") {
                STATIC_REQUIRE(
                    std::same_as<traits<one_shot_ints>::iterator_concept, std::input_iterator_tag>);
                STATIC_REQUIRE(std::same_as<traits<forward_ints>::iterator_concept,
                    std::forward_iterator_tag>);
                STATIC_REQUIRE(std::same_as<traits<bidir_ints>::iterator_concept,
                    std::bidirectional_iterator_tag>);
                STATIC_REQUIRE(std::same_as<traits<rand_ints>::iterator_concept,
                    std::random_access_iterator_tag>);
            }
        }
    }
}
