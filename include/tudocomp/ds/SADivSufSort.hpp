#pragma once

#include <tudocomp/ds/CompressMode.hpp>
#include <tudocomp/ds/ArrayDS.hpp>
#include <tudocomp/util/divsufsort.hpp>

namespace tdc {

/// Constructs the suffix array using divsufsort.
class SADivSufSort : public ArrayDS {
public:
    inline static Meta meta() {
        Meta m("sa", "divsufsort");
        return m;
    }

    template<typename textds_t>
    inline SADivSufSort(Env&& env, const textds_t& t, CompressMode cm)
        : ArrayDS(std::move(env)) {

        this->env().begin_stat_phase("Construct SA");

        // Allocate
        const size_t n = t.size();
        const size_t w = bits_for(n);

        // divsufsort needs one additional bit for signs
        m_data = std::make_unique<iv_t>(n, 0,
            (cm == CompressMode::direct) ? w + 1 : LEN_BITS);

        // Use divsufsort to construct
        divsufsort(t.text(), *m_data, n);

        if(cm == CompressMode::direct || cm == CompressMode::delayed) {
            compress();
        }

        this->env().log_stat("bit_width", size_t(m_data->width()));
        this->env().log_stat("size", m_data->bit_size() / 8);
        this->env().end_stat_phase();
    }

    void compress() {
        DCHECK(m_data);
        m_data->width(bits_for(m_data->size()));
        m_data->shrink_to_fit();
    }
};

} //ns