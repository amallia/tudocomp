#pragma once

#include <tudocomp/pre_header/RegistryOf.hpp>
#include <tudocomp/pre_header/Env.hpp>
#include <tudocomp/EnvRoot.hpp>

/// \cond INTERNAL
namespace tdc {

template<typename algorithm_t>
template<typename T>
inline void RegistryOf<algorithm_t>::register_algorithm() {
    auto meta = T::meta();

    ast::Value s = std::move(meta).build_static_args_ast_value();

    gather_types(m_data->m_algorithms, std::move(meta));

    auto static_s
        = eval::pattern_eval(std::move(s), root_type(), m_data->m_algorithms);

    CHECK(m_data->m_registered.count(static_s) == 0) << "registered twice"; // Don't register twice...
    m_data->m_registered[std::move(static_s)] = [](Env&& env) {
        return std::make_unique<T>(std::move(env));
    };
}

template<typename algorithm_t>
inline eval::AlgorithmTypes& RegistryOf<algorithm_t>::algorithm_map() {
    return m_data->m_algorithms;
}

template<typename algorithm_t>
inline const eval::AlgorithmTypes& RegistryOf<algorithm_t>::algorithm_map() const {
    return m_data->m_algorithms;
}

template<typename algorithm_t>
inline std::vector<pattern::Algorithm> RegistryOf<algorithm_t>::all_algorithms_with_static_internal(
    std::vector<AlreadySeenPair>& already_seen,
    View type
) const {
    std::vector<pattern::Algorithm> r;

    using AlgorithmArgs = std::vector<pattern::Arg>;

    for (auto& c : m_data->m_algorithms.at(type)) {
        std::vector<std::vector<AlgorithmArgs>> args_variations;

        auto seen = AlreadySeenPair {
            type, c.name()
        };
        //std::cout << "Checking if " << type << ", " << c.name() << "\n";
        int found = 0;
        for (auto& x: already_seen) {
            //std::cout << "  " << x.pair[0] << "\n";
            if (x.pair[0] == type) {
                found++;
            }
        }
        if (found > 2) {
            //std::cout << "break\n";
            continue;
        }

        already_seen.push_back(seen);

        for (auto& arg : c.arguments()) {
            const std::string& arg_name = arg.name();
            CHECK(arg_name.size() > 0);

            auto arg_type = arg.type();
            bool is_static = arg.is_static();
            if (is_static) {
                std::vector<AlgorithmArgs> arg_variations;

                for(auto arg : all_algorithms_with_static_internal(already_seen, arg_type)) {
                    arg_variations.push_back(AlgorithmArgs {
                        pattern::Arg {
                            std::string(arg_name),
                            std::move(arg)
                        }
                    });
                }
                args_variations.push_back(arg_variations);
            }
        }

        already_seen.pop_back();

        std::string x_name;
        std::vector<pattern::Arg> x_args;

        x_name = c.name();
        std::vector<AlgorithmArgs> r_;
        if (args_variations.size() == 0) {
            pattern::Algorithm x {
                std::move(x_name),
                std::move(x_args)
            };

            r.push_back(x);
        } else {
            r_ = cross<AlgorithmArgs>(
                std::move(args_variations), [](AlgorithmArgs s,
                                                AlgorithmArgs& t) {
                for (auto& e : t) {
                    s.push_back(e);
                }
                return s;
            });
        }

        for (auto& elem : r_) {
            r.push_back(pattern::Algorithm {
                std::string(c.name()),
                std::move(elem)
            });
        }
    }

    return r;
}

template<typename algorithm_t>
inline std::vector<pattern::Algorithm> RegistryOf<algorithm_t>::all_algorithms_with_static(View type) const {
    std::vector<pattern::Algorithm> filtered_r;

    std::vector<AlreadySeenPair> already_seen;
    for (auto x : all_algorithms_with_static_internal(already_seen, type)) {
        if (m_data->m_registered.count(x) > 0) {
            filtered_r.push_back(std::move(x));
        }
    }

    return filtered_r;
}

template<typename algorithm_t>
inline std::vector<pattern::Algorithm> RegistryOf<algorithm_t>::check_for_undefined_algorithms() {
    std::vector<pattern::Algorithm> r;
    for (auto& s : all_algorithms_with_static(root_type())) {
        if (m_data->m_registered.count(s) == 0) {
            r.push_back(s);
        }
    }
    return r;
}

template<typename algorithm_t>
inline RegistryOf<algorithm_t> RegistryOf<algorithm_t>::with_all_from(std::function<void(RegistryOf&)> f) {
    RegistryOf r;
    f(r);
    return r;
}

template<typename algorithm_t>
inline std::string RegistryOf<algorithm_t>::generate_doc_string(const std::string& title) const {
    auto print = [](std::vector<decl::Algorithm>& x, size_t iden) {
        std::vector<std::string> cells;

        for (auto& y : x) {
            auto spec = y.to_string(true);

            std::stringstream where;
            bool first = true;
            for (auto& z : y.arguments()) {
                if (first) {
                    where << "\n  where ";
                } else {
                    where << "\n        ";
                }
                first = false;
                where << "`" << z.name() << "` is one of [" << z.type() << "],";
            }
            auto s = spec + where.str();
            if (y.arguments().size() > 0) {
                s = s.substr(0, s.size() - 1);
            }
            cells.push_back(s);
            cells.push_back(y.doc());
        }

        return indent_lines(make_table(cells, 2), iden);
    };

    std::stringstream ss;

    ss << "  [" << title << "]\n";
    ss << print(m_data->m_algorithms[root_type()], 2) << "\n\n";

    ss << "  [Argument types]\n";
    for (auto& x : m_data->m_algorithms) {
        if (x.first == root_type()) {
            continue;
        }
        ss << "    [" << x.first << "]\n";
        ss << print(x.second, 4) << "\n\n";
    }

    return ss.str();
}

template<typename algorithm_t>
inline std::unique_ptr<algorithm_t> RegistryOf<algorithm_t>::select_algorithm(EnvRoot env_root, AlgorithmValue const& algo) const {
    auto& static_only_evald_algo = algo.static_selection();

    if (m_data->m_registered.count(static_only_evald_algo) > 0) {
        auto& constructor = m_data->m_registered[static_only_evald_algo];
        return constructor(Env(env_root, algo));
    } else {
        throw std::runtime_error("No implementation found for " + std::string(root_type()) + " "
        + static_only_evald_algo.to_string()
        );
    }
}

template<typename algorithm_t>
inline std::unique_ptr<algorithm_t> RegistryOf<algorithm_t>::create_algorithm(AlgorithmValue const& algo) const {
    Registry registry = m_data->m_registry;
    auto env_root = EnvRoot(registry, AlgorithmValue(algo));
    return select_algorithm(env_root, algo);
}

template<typename algorithm_t>
inline std::unique_ptr<algorithm_t> RegistryOf<algorithm_t>::create_algorithm(std::string const& algo) const {
    auto av = parse_algorithm_id(algo);
    return create_algorithm(av);
}

template<typename algorithm_t>
inline AlgorithmValue RegistryOf<algorithm_t>::parse_algorithm_id(
    string_ref text) const {

    ast::Parser p { text };
    auto parsed_algo = p.parse_value();
    auto options = eval::cl_eval(std::move(parsed_algo),
                                    root_type(),
                                    m_data->m_algorithms);

    return std::move(options).to_algorithm();
}

}
/// \endcond
