#pragma once

#include "common.h"
#include "formula.h"


#include <unordered_set>
#include <optional>
#include <functional>//предложено
#include <stack>

class Sheet;

namespace Impl {

    class Impl {
    public:
        virtual ~Impl() = default;
        virtual CellInterface::Value GetValue() const = 0;
        virtual std::string GetText() const = 0;
    };

    class EmptyImpl : public Impl {
    public:
        EmptyImpl() = default;
        CellInterface::Value GetValue() const override;
        std::string GetText() const override;
    };

    class TextImpl : public Impl {
    public:
        TextImpl(std::string text, bool apostrof = false);
        CellInterface::Value GetValue() const override;
        std::string GetText() const override;
    private:
        std::string str_;
        bool apostrof_;
    };

    class FormulaImpl : public Impl {
    public:
        FormulaImpl(std::string text, Sheet& sheet);
        CellInterface::Value GetValue() const override;
        std::string GetText() const override;
        std::vector<Position> GetReferencedCells() ;
        void InvalidCashe();

    private:
        std::unique_ptr<FormulaInterface> formula_;
        mutable std::optional<FormulaInterface::Value> cashe_;
        Sheet& sheet_;
    };
}

class Cell : public CellInterface {
public:
    Cell(Sheet& sheet, std::string text, Position pos);
    Cell(Sheet& sheet, Position pos);

    ~Cell();

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

    bool IsReferenced() const; 

private:
    std::unique_ptr<Impl::Impl> impl_;
    std::unordered_set<Position, PositionHasher> children_;
    std::unordered_set<Position, PositionHasher> parents_;
    Sheet& sheet_;
    Position pos_;

    void CheckCyclicDependencies(std::unordered_set<Position, PositionHasher>& cells, std::stack<Position>& cell_stack,
        std::unordered_set<Position, PositionHasher>& cell_set, std::vector<Position> ref_cells) const;

    void InvalidCasheCells(std::unordered_set<Position, PositionHasher>,const Position& pos, bool first_iteration = false);

    void UpDateImplAfterSet(std::unique_ptr<Impl::Impl>&& impl_);
};


