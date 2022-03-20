/*  Binary Matrix (internal template)
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#ifndef PokemonAutomation_Kernels_BinaryMatrix_t_H
#define PokemonAutomation_Kernels_BinaryMatrix_t_H

#include "Common/Cpp/Exceptions.h"
#include "Kernels_PackedBinaryMatrixCore.h"
#include "Kernels_SparseBinaryMatrixCore.h"
#include "Kernels_BinaryMatrix.h"

namespace PokemonAutomation{
namespace Kernels{



template <typename Tile>
class PackedBinaryMatrix_t : public PackedBinaryMatrix_IB{
public:
    PackedBinaryMatrix_t() = default;
    PackedBinaryMatrix_t(size_t width, size_t height)
        : m_matrix(width, height)
    {}

    const PackedBinaryMatrixCore<Tile>& get() const{ return m_matrix; }
          PackedBinaryMatrixCore<Tile>& get()      { return m_matrix; }

    virtual BinaryMatrixType type() const override{ return Tile::TYPE; }
    virtual std::unique_ptr<PackedBinaryMatrix_IB> clone() const override{
        auto ret = std::make_unique<PackedBinaryMatrix_t>();
        ret->m_matrix = m_matrix;
        return ret;
    }

    virtual void clear() override{ m_matrix.clear(); }

    virtual void set_zero() override{ m_matrix.set_zero(); }
    virtual void set_ones() override{ m_matrix.set_ones(); }
    virtual void invert() override{ m_matrix.invert(); }

    virtual std::string dump() const override{ return m_matrix.dump(); }
    virtual std::string dump(size_t min_x, size_t min_y, size_t max_x, size_t max_y) const override{ return m_matrix.dump(min_x, min_y, max_x, max_y); }
    virtual std::string dump_tiles() const override{ return m_matrix.dump_tiles(); }

public:
    virtual size_t width() const override{ return m_matrix.width(); }
    virtual size_t height() const override{ return m_matrix.height(); }

    //  These are slow.
    virtual bool get(size_t x, size_t y) const override{ return m_matrix.get(x, y); }
    virtual void set(size_t x, size_t y, bool set) override{ m_matrix.set(x, y, set); }

    virtual std::unique_ptr<PackedBinaryMatrix_IB> submatrix(size_t x, size_t y, size_t width, size_t height) const override{
        auto ret = std::make_unique<PackedBinaryMatrix_t>();
        ret->m_matrix = m_matrix.submatrix(x, y, width, height);
        return ret;
    }

private:
    PackedBinaryMatrixCore<Tile> m_matrix;
};


template <typename Tile>
class SparseBinaryMatrix_t : public SparseBinaryMatrix_IB{
public:
    SparseBinaryMatrix_t() = default;
    SparseBinaryMatrix_t(size_t width, size_t height)
        : m_matrix(width, height)
    {}

    const SparseBinaryMatrixCore<Tile>& get() const{ return m_matrix; }
          SparseBinaryMatrixCore<Tile>& get()      { return m_matrix; }

    virtual BinaryMatrixType type() const override{ return Tile::TYPE; }
    virtual std::unique_ptr<SparseBinaryMatrix_IB> clone() const override{
        auto ret = std::make_unique<SparseBinaryMatrix_t>();
        ret->m_matrix = m_matrix;
        return ret;
    }

    virtual void clear() override{ m_matrix.clear(); }

    virtual void operator|=(const SparseBinaryMatrix_IB& x) override{
        if (type() != x.type()){
            throw InternalProgramError(nullptr, PA_CURRENT_FUNCTION, "Mismatching matrix types.");
        }
        m_matrix |= static_cast<const SparseBinaryMatrix_t&>(x).m_matrix;
    }

    virtual std::string dump() const override{ return m_matrix.dump(); }
    virtual std::string dump(size_t min_x, size_t min_y, size_t max_x, size_t max_y) const override{ return m_matrix.dump(min_x, min_y, max_x, max_y); }

public:
    virtual size_t width() const override{ return m_matrix.width(); }
    virtual size_t height() const override{ return m_matrix.height(); }

    //  These are slow.
    virtual bool get(size_t x, size_t y) const override{ return m_matrix.get(x, y); }
    virtual void set(size_t x, size_t y, bool set) override{ m_matrix.set(x, y, set); }

    virtual std::unique_ptr<PackedBinaryMatrix_IB> submatrix(size_t x, size_t y, size_t width, size_t height) const override{
        auto ret = std::make_unique<PackedBinaryMatrix_t<Tile>>();
        ret->get() = m_matrix.submatrix(x, y, width, height);
        return ret;
    }

private:
    SparseBinaryMatrixCore<Tile> m_matrix;
};





}
}
#endif
