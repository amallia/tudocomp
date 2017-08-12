#include <glog/logging.h>
#include <gtest/gtest.h>

#include <tudocomp/meta/AlgorithmDecl.hpp>
#include <tudocomp/meta/ast/TypeConversion.hpp>
#include <tudocomp/meta/ast/Parser.hpp>
#include <tudocomp/meta/AlgorithmConfig.hpp>
#include <tudocomp/meta/Meta.hpp>

using namespace tdc::meta;

constexpr TypeDesc coder_td("coder");
constexpr TypeDesc compressor_td("coder");

class OtherAlgo {
public:
    static inline Meta meta() {
        return Meta("other", TypeDesc("other"), "Any other algorithm.");
    }
};

class BinaryCoder {
public:
    static inline Meta meta() {
        return Meta("binary", coder_td, "Binary coder.");
    }
};

class UnaryCoder {
public:
    static inline Meta meta() {
        return Meta("unary", coder_td, "Unary coder.");
    }
};

template<typename coder_t, typename coder2_t>
class LZ77Compressor {
public:
    static inline Meta meta() {
        Meta m("lz77", compressor_td, "LZ77 online compressor.");
        m.param("coder").strategy<coder_t>(coder_td, Meta::Default<BinaryCoder>());
        m.param("coders").strategy_list<coder_t, coder2_t>(
            coder_td, Meta::Defaults<BinaryCoder, UnaryCoder>());
        m.param("window").primitive(10);
        m.param("values").primitive_list({1,4,7});
        return m;
    }
};

TEST(Sandbox, example) {
    // registry
    AlgorithmLib lib;
    lib.emplace("other", OtherAlgo::meta().decl());
    lib.emplace("binary", BinaryCoder::meta().decl());
    lib.emplace("unary", UnaryCoder::meta().decl());
    lib.emplace("lz77", LZ77Compressor<BinaryCoder, UnaryCoder>::meta().decl());

    // meta signatures
    DLOG(INFO) << "signature(binary, binary): " <<
        LZ77Compressor<BinaryCoder, BinaryCoder>::meta().signature()->str();
    DLOG(INFO) << "signature(binary, unary): " <<
        LZ77Compressor<BinaryCoder, UnaryCoder>::meta().signature()->str();
    DLOG(INFO) << "signature(unary, binary): " <<
        LZ77Compressor<UnaryCoder, BinaryCoder>::meta().signature()->str();
    DLOG(INFO) << "signature(unary, unary): " <<
        LZ77Compressor<UnaryCoder, UnaryCoder>::meta().signature()->str();

    // parse
    /*DLOG(INFO) << "parse...";
    auto v = ast::Parser::parse("lz77()");
    DLOG(INFO) << v->str();

    // attempt to create config
    DLOG(INFO) << "config...";
    auto cfg = AlgorithmConfig(lib.at("lz77"), v, lib);
    DLOG(INFO) << cfg.str();

    // signature
    DLOG(INFO) << "signature...";
    DLOG(INFO) << cfg.signature()->str();*/
}