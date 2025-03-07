#pragma once

#include <array>
#include <concepts>
#include <cstdint>
#include <limits>
#include <span>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "noctern/meta.hpp"

namespace noctern {
    template <typename T, typename InternIndexType>
        requires std::unsigned_integral<InternIndexType>
    class intern_table;

    namespace intern_table_internal {
        template <typename T, typename InternIndexType>
            requires std::unsigned_integral<InternIndexType>
        class interned {
            friend intern_table<T, InternIndexType>;
            friend std::hash<interned>;

        public:
            constexpr interned(const interned&) = default;
            constexpr interned(interned&&) noexcept = default;

            friend constexpr bool operator==(interned, interned) = default;

            template <typename Word>
                requires std::integral<Word>
            static constexpr auto encoded_size(type_t<Word>) {
                constexpr auto Div = sizeof(InternIndexType) / sizeof(Word);
                constexpr auto Rem = sizeof(InternIndexType) % sizeof(Word);
                constexpr auto DivRoundingUp = Div + (Rem != 0 ? 1 : 0);
                return DivRoundingUp;
            }

            template <typename Word>
                requires std::integral<Word>
            constexpr auto encode_as(type_t<Word>) const {
                constexpr auto MaxWord = std::numeric_limits<Word>::max();
                constexpr auto WordBits = std::numeric_limits<Word>::digits;

                std::array<Word, encoded_size(type<Word>)> result {};
                InternIndexType data = index_;
                for (size_t index = 0; index < result.size(); ++index) {
                    result[index] = data & MaxWord;
                    // TODO: disable -Wmissing-field-initializers
                    data >>= WordBits;
                }
                return result;
            }

            template <typename Word>
                requires std::integral<Word>
            static constexpr interned decode_from(std::span<const Word> data) {
                constexpr auto WordBits = std::numeric_limits<Word>::digits;

                InternIndexType result = 0;
                for (size_t indexp1 = encoded_size(type<Word>); indexp1 > 0; --indexp1) {
                    result <<= WordBits;
                    result |= data[indexp1 - 1];
                }

                return interned {result};
            }

        private:
            explicit constexpr interned(InternIndexType index)
                : index_(index) {
            }

            InternIndexType index_;
        };
    }

    template <typename T, typename InternIndexType = uint32_t>
        requires std::unsigned_integral<InternIndexType>
    class intern_table {
    public:
        using interned = intern_table_internal::interned<T, InternIndexType>;

        const T& get(interned token) const {
            return data_[token.index_];
        }

        interned intern(const T& item) {
            InternIndexType index = table_.size();

            auto [it, was_inserted] = table_.try_emplace(item, index);
            if (was_inserted) {
                data_.push_back(item);
            } else {
                index = it->second;
            }

            return interned {index};
        }

    private:
        std::vector<T> data_;
        std::unordered_map<T, InternIndexType> table_;
    };

    using string_intern_table = intern_table<std::string_view>;
    using interned_string = string_intern_table::interned;
}

template <typename T, typename InternIndexType>
struct std::hash<typename noctern::intern_table_internal::interned<T, InternIndexType>> {
    constexpr size_t operator()(
        typename noctern::intern_table_internal::interned<T, InternIndexType> token) const {
        return std::hash<InternIndexType> {}(token.index_);
    }
};
