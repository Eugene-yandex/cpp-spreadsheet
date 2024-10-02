#include "cell.h"
#include "sheet.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>
#include <stack>

namespace Impl {

    CellInterface::Value EmptyImpl::GetValue() const {
        return 0.0;
    }

    std::string EmptyImpl::GetText() const {
        using namespace std::string_literals;
        return ""s;
    }

    TextImpl::TextImpl(std::string text, bool apostrof) :
        str_(std::move(text)), apostrof_(apostrof) {

    }

    CellInterface::Value TextImpl::GetValue() const {
        if (apostrof_) {
            return str_.substr(1);
        }
        return str_;
    }

    std::string TextImpl::GetText() const {
        return str_;
    }

    FormulaImpl::FormulaImpl(std::string text, Sheet& sheet) :
       formula_(ParseFormula(std::move(text))), sheet_(sheet) {

    }

    CellInterface::Value FormulaImpl::GetValue() const {
        if (!cashe_) {
            cashe_ = formula_->Evaluate(sheet_); 
        }
        
        if (std::holds_alternative<double>(cashe_.value())) {
            return std::get<double>(cashe_.value());
        }
        else {
            return std::get<FormulaError>(cashe_.value());
        }
    }

    std::string FormulaImpl::GetText() const {
        return std::string('=' + formula_->GetExpression());
    }

    std::vector<Position> FormulaImpl::GetReferencedCells()  {
        return (*formula_).GetReferencedCells();
    }

    void FormulaImpl::InvalidCashe(){
        cashe_ = std::nullopt;
    }

}

Cell::Cell(Sheet& sheet, std::string text, Position pos) :
    sheet_(sheet), pos_(pos) {
    Set(std::move(text));
}

Cell::Cell(Sheet& sheet, Position pos) :
    sheet_(sheet), pos_(pos) {
}

Cell::~Cell() = default;

void Cell::Set(std::string text) {
    if (text[0] == ESCAPE_SIGN) {
        auto new_impl = std::make_unique<Impl::TextImpl>(std::move(text), true);
        UpDateImplAfterSet(std::move(new_impl));
    }
    else if (text[0] == FORMULA_SIGN) {
        if (text.length() == 1) {
            auto new_impl = std::make_unique<Impl::TextImpl>(std::move(text));            
            UpDateImplAfterSet(std::move(new_impl));
        }
        else {
            auto new_impl = std::make_unique<Impl::FormulaImpl>(text.substr(1),sheet_);
            std::vector<Position> ref_cells = dynamic_cast<Impl::FormulaImpl*>(new_impl.get())->GetReferencedCells();
            if (!ref_cells.empty()) {
                std::stack<Position> cell_stack;
                std::unordered_set<Position, PositionHasher> cell_set;
                std::unordered_set<Position, PositionHasher> all_cells;
                cell_set.insert(pos_);
                cell_stack.push(pos_);
                while (cell_set.size() != 0) {
                    CheckCyclicDependencies(all_cells, cell_stack, cell_set, ref_cells);
                }

                for (const auto& pos : children_) {
                    Cell* refrenced = dynamic_cast<Cell*>(sheet_.GetCell(pos));
                    refrenced->parents_.insert(pos_);
                }
                children_.clear(); 

                for (const auto& pos : ref_cells) {
                    Cell* refrenced = dynamic_cast<Cell*>(sheet_.GetCell(pos));
                    if (!refrenced) {
                        sheet_.SetCell(pos, "");
                        refrenced = dynamic_cast<Cell*>(sheet_.GetCell(pos));
                    }
                    refrenced->parents_.insert(pos_);
                    children_.insert(refrenced->pos_);
                }
            }
            InvalidCasheCells(parents_, pos_, true);
            impl_.reset();
            impl_ = std::move(new_impl);

            if (impl_) {
                Impl::FormulaImpl* formula_impl = dynamic_cast<Impl::FormulaImpl*>(impl_.get());
                if (formula_impl) {
                    formula_impl->InvalidCashe();
                    formula_impl->GetValue();
                }
            }
        }
    }
    else {
        auto new_impl = std::make_unique<Impl::TextImpl>(std::move(text));
        UpDateImplAfterSet(std::move(new_impl));
    }
   
}


void Cell::Clear() {
    impl_.reset();
}

Cell::Value Cell::GetValue() const {
    return impl_->GetValue();
}

std::string Cell::GetText() const {
    return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
    std::vector<Position> referenced_cells;
    if (IsReferenced()) {
        return dynamic_cast<Impl::FormulaImpl*>(impl_.get())->GetReferencedCells();
    }
    return referenced_cells;
}

bool Cell::IsReferenced() const {
    if (!children_.empty()) {
        return true;
    }
    return false;
}

void Cell::CheckCyclicDependencies(std::unordered_set<Position, PositionHasher>& cells, 
    std::stack<Position>& cell_stack, std::unordered_set<Position, PositionHasher>& cell_set, std::vector<Position> ref_cells) const {
    for (const auto& pos : ref_cells) {
        if (cells.count(pos) == 0) {
            cells.insert(pos);
            if (cell_set.count(pos) > 0) {
                throw CircularDependencyException("Cyclic dependencies");
            }
            const auto& cell = sheet_.GetCell(pos);
            if(cell && (!cell->GetReferencedCells().empty())) {
                CheckCyclicDependencies(cells, cell_stack, cell_set, sheet_.GetCell(pos)->GetReferencedCells());
            }
            else {
                const auto& cell = cell_stack.top();
                cell_set.erase(cell);
                cell_stack.pop();
            }
        }
        else {
            const auto& cell = cell_stack.top();
            cell_set.erase(cell);
            cell_stack.pop();
        }

    }
}

void Cell::InvalidCasheCells(std::unordered_set<Position, PositionHasher> cells, const Position& posish, bool first_iteration) {
    for (const auto& pos : cells) {
        Cell* cell_ptr = dynamic_cast<Cell*>(sheet_.GetCell(pos));
        Impl::FormulaImpl* formula_impl = dynamic_cast<Impl::FormulaImpl*>
            (cell_ptr->impl_.get());
        if (formula_impl) {
            formula_impl->InvalidCashe();
        }
        if (!cell_ptr->parents_.empty()) {
            InvalidCasheCells(cell_ptr->parents_, posish);
        }
        if (first_iteration && cell_ptr->children_.count(posish) > 0) {
            children_.erase(posish);
        }
    }
}

void Cell::UpDateImplAfterSet(std::unique_ptr<Impl::Impl>&& impl) {
    impl_.reset();
    impl_ = std::move(impl);
    children_.clear();
    InvalidCasheCells(parents_, pos_, true);
}

