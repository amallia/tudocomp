#pragma once

#include <tudocomp/util.hpp>
#include <tudocomp/Env.hpp>
#include <tudocomp/Compressor.hpp>
#include <tudocomp/ds/IntVector.hpp>

#include <tudocomp/util/View.hpp>
#include <tudocomp/compressors/esp/pre_header.hpp>
#include <tudocomp/compressors/esp/meta_blocks.hpp>

namespace tdc {
namespace esp {
    using in_t = ConstGenericView<size_t>;

    template<size_t N>
    struct Array {
        std::array<size_t, N> m_data;
        Array() {
            for(size_t i = 0; i < N; i++) {
                m_data[i] = 0;
            }
        }
        Array(in_t v) {
            DCHECK_EQ(v.size(), N);
            for(size_t i = 0; i < N; i++) {
                m_data[i] = v[i];
            }
        }
    };

    template<size_t N>
    bool operator==(const Array<N>& lhs, const Array<N>& rhs) {
        for(size_t i = 0; i < N; i++) {
            if (lhs.m_data[i] != rhs.m_data[i]) return false;
        }
        return true;
    }
}
}
namespace std {
    template<size_t N>
    struct hash<tdc::esp::Array<N>>
    {
        size_t operator()(const tdc::esp::Array<N>& x) const {
            return std::hash<tdc::ConstGenericView<size_t>>()(
                tdc::ConstGenericView<size_t> {
                    x.m_data.data(),
                    x.m_data.size()
                });
        }
    };
}

namespace tdc {
namespace esp {

    struct GrammarRules {
        GrammarRules() {}
        using a2_t = std::unordered_map<Array<2>, size_t>;
        using a3_t = std::unordered_map<Array<3>, size_t>;

        a2_t n2;
        a3_t n3;

        size_t counter = 1;

        size_t add(in_t v) {
            size_t* r;
            if (v.size() == 2) {
                r = &n2[v];
            } else if (v.size() == 3) {
                r = &n3[v];
            } else {
                DCHECK(false);
            }

            if (*r == 0) {
                *r = counter++;
            }

            return *r - 1;
        }
    };

    struct Round {
        GrammarRules gr;
        size_t alphabet;
        std::vector<size_t> string;
    };

    std::vector<Round> generate_grammar_rounds(string_ref input,
                                               bool silent = false) {
        std::vector<Round> rounds;

        // Initialize initial round
        {
            Round round0 {
                GrammarRules(),
                256,
                std::vector<size_t>(),
            };
            round0.string.reserve(input.size());
            for (auto c : input) {
                round0.string.push_back(c);
            }
            rounds.push_back(std::move(round0));
        }

        for(size_t n = 0;; n++) {
            if (!silent) std::cout << "\n[Round " << n << "]:\n\n";
            Round& r = rounds.back();

            if (r.string.size() == 1) {
                if (!silent) std::cout << "Done\n";
                break;
            }

            in_t in = r.string;
            std::vector<size_t> new_layer;

            esp::Context<in_t> ctx {
                r.alphabet,
                in,
            };
            if (!silent) {
                ctx.print_mb2_trace = false;
                ctx.print_only_adjusted = true;
            } else {
                ctx.print_mb2_trace = false;
                ctx.print_only_adjusted = true;
                ctx.print_mb_trace = false;
            }

            esp::split(in, ctx);
            if (!silent) std::cout << "[Adjusted]:\n";
            const auto& v = ctx.adjusted_blocks();

            if (!silent) std::cout << "\n[Rules]:\n";

            {
                in_t s = in;
                for (auto e : v) {
                    auto slice = s.slice(0, e.len);
                    s = s.slice(e.len);
                    if (!silent) std::cout <<  "  "
                        << debug_p(slice, r.alphabet);

                    auto rule_name = r.gr.add(slice);
                    new_layer.push_back(rule_name);
                    if (!silent) std::cout << " -> " << rule_name << "\n";
                }
            }
            new_layer.shrink_to_fit();

            // Prepare next round
            {
                rounds.push_back(Round {
                    GrammarRules(),
                    r.gr.counter,
                    std::move(new_layer),
                });
            }
        }

        return std::move(rounds);
    }

    ///
    /// REFACTOR: ONE GLOBAL HASHMAP STRUCTURE WITH GLOBAL ARRAY INDICE CONTENTS
    /// SUBTRACT DOWN TO LAYER_INDEX MANUALLY WHERE NEEDED
    ///

    bool MAKE_BINARY = true;
    size_t GRAMMAR_PD_ELLIDED_PREFIX = 256;

    void make_binary(std::vector<std::vector<size_t>>& rules) {
        for (size_t i = 0; i < rules.size(); i++) {
            if (rules[i].size() == 3) {
                auto char1 = rules[i][0];
                auto char2 = rules[i][1];
                auto char3 = rules[i][2];

                auto new_rule = rules.size() + GRAMMAR_PD_ELLIDED_PREFIX;
                rules.push_back(std::vector<size_t> { char1, char2 });

                rules[i].clear();
                rules[i].push_back(new_rule);
                rules[i].push_back(char3);
            }
        }
    }

    std::vector<std::vector<size_t>> generate_grammar(const std::vector<Round>& rs) {
        size_t offset = 256;
        size_t prev_offset = 0;
        std::vector<std::vector<size_t>> ret;
        for (auto& r: rs) {
            ret.resize(offset - GRAMMAR_PD_ELLIDED_PREFIX + r.gr.counter - 1);

            auto doit = [&](auto& n) {
                for (auto& k: n) {
                    auto& a = k.first.m_data;
                    std::vector<size_t> v(a.begin(), a.end());
                    for (auto& e : v) {
                        e += (prev_offset);
                    }

                    auto& er = ret[offset - GRAMMAR_PD_ELLIDED_PREFIX + k.second - 1];
                    DCHECK(er.empty());
                    er = std::move(v);
                }
            };

            doit(r.gr.n2);
            doit(r.gr.n3);
            prev_offset = offset;
            offset += r.gr.counter - 1;
        }

        if (MAKE_BINARY) {
            make_binary(ret);
        }

        return std::move(ret);
    }

    void derive_text_rec(const std::vector<std::vector<size_t>>& rules,
                         std::ostream& o,
                         size_t rule) {
        if (rule < 256) {
            o << char(rule);
        } else for (auto r : rules[rule - GRAMMAR_PD_ELLIDED_PREFIX]) {
            derive_text_rec(rules, o, r);
        }
    }

    std::string derive_text(const std::vector<std::vector<size_t>>& rules) {
        std::stringstream ss;

        derive_text_rec(rules, ss, rules.size() - 1 + GRAMMAR_PD_ELLIDED_PREFIX);

        return ss.str();
    }


}
}
