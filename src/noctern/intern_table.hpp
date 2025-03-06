#pragma once

#include <cstdint>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace noctern {
    template <typename T, typename InternIndexType>
    class intern_table;

    namespace intern_table_internal {
        template <typename T, typename InternIndexType>
        class interned {
            friend intern_table<T, InternIndexType>;
            friend std::hash<interned>;

        public:
            friend bool operator==(interned, interned) = default;

        private:
            constexpr interned(InternIndexType index)
                : index_(index) {
            }

            InternIndexType index_;
        };
    }

    template <typename T, typename InternIndexType = uint32_t>
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
