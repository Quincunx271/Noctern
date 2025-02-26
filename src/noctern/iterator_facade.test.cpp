#include "./iterator_facade.hpp"

#include <catch2/catch.hpp>

namespace noctern {
    namespace {
        class one_shot_ints : public iterator_facade<one_shot_ints> {
        public:
            using iterator_category = std::input_iterator_tag;
            using difference_type = int;

            constexpr int read() const {
                return value_;
            }

            constexpr void advance(val_t<1>) {
                ++value_;
            }

        private:
            int value_;
        };
        static_assert(std::input_iterator<one_shot_ints>);

        class forward_ints : public iterator_facade<forward_ints> {
        public:
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
