#include "./interpreter.hpp"

#include <charconv>

#include "noctern/enum.hpp"
#include "noctern/tokenize.hpp"

namespace noctern {
    namespace {
        double parse_double(std::string_view value) {
            double answer;
            auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), answer);
            assert(ptr == value.data() + value.size());
            assert(ec == std::errc {});
            return answer;
        }
    }

    double interpreter::eval_fn(const tokens& source, token from) const {
        frame frame;
        auto pos = source.to_iterator(from);

        token_id id = source.id(*pos);
        if (id == token_id::lbrace) {
            return eval_block(source, frame, pos);
        } else {
            return eval_expr(source, frame, pos);
        }
    }

    double interpreter::eval_block(
        const tokens& source, frame& frame, tokens::const_iterator& pos) const {
        assert(source.id(*pos) == token_id::lbrace);
        ++pos;

        while (source.id(*pos) == token_id::valdef_intro) {
            const token ident = *pos;
            ++pos;

            frame.locals[source.string(ident)] = eval_expr(source, frame, pos);
        }

        assert(source.id(*pos) == token_id::return_);
        ++pos;
        double result = eval_expr(source, frame, pos);
        assert(source.id(*pos) == token_id::rbrace);
        ++pos;
        return result;
    }

    double interpreter::eval_expr(
        const tokens& source, frame& frame, tokens::const_iterator& pos) const {
        assert(frame.expr_stack.empty());

        while (source.id(*pos) != token_id::statement_end) {
            token next = *pos;
            token_id id = source.id(next);
            ++pos;

            if (id == token_id::ident) {
                auto local = frame.locals.find(source.string(next));
                assert(local != frame.locals.end() && "Unknown identifier");
                frame.expr_stack.push_back(local->second);
            } else if (id == token_id::int_lit || id == token_id::real_lit) {
                frame.expr_stack.push_back(parse_double(source.string(next)));
            } else if (id == token_id::plus || id == token_id::minus || id == token_id::mult
                || id == token_id::div) {
                assert(frame.expr_stack.size() >= 2);
                double second = frame.expr_stack.back();
                frame.expr_stack.pop_back();
                double first = frame.expr_stack.back();
                frame.expr_stack.pop_back();

                double result = enum_switch(id, [first, second]<token_id id>(val_t<id>) -> double {
                    if constexpr (id == token_id::plus) {
                        return first + second;
                    } else if constexpr (id == token_id::minus) {
                        return first - second;
                    } else if constexpr (id == token_id::mult) {
                        return first * second;
                    } else if constexpr (id == token_id::div) {
                        return first / second;
                    } else {
                        assert(false && "not an operation");
                    }
                });

                frame.expr_stack.push_back(result);
            }
        }
        ++pos;

        assert(frame.expr_stack.size() == 1);
        double result = frame.expr_stack.back();
        frame.expr_stack.pop_back();
        return result;
    }
}
