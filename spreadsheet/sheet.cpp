#include "sheet.h"

#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

inline std::ostream& operator<<(std::ostream& output, const CellInterface::Value& value) {
    std::visit([&](const auto& x) { output << x; }, value);
    return output;
}

Sheet::Sheet() = default;
Sheet::~Sheet() = default;

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("incorrect position");
    }

    if (field_.count(pos) > 0) {
        field_.at(pos)->Set(std::move(text));
    }
    else {
        field_.insert({ pos,std::make_unique<Cell>(*this, std::move(text), pos) });
    }
}

const CellInterface* Sheet::GetCell(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("incorrect position");
    }
    if (field_.count(pos) > 0) {
        return field_.at(pos).get();
    }
    return nullptr;
}
CellInterface* Sheet::GetCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("incorrect position");
    }
    if (field_.count(pos) > 0) {
        return field_.at(pos).get();
    }
    return nullptr;
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("incorrect position");
    }
    if (field_.count(pos) > 0) {
        field_.erase(pos);
    }
}

Size Sheet::GetPrintableSize() const {
    if (field_.size() != 0) {
        int row = 0;
        int col = 0;
        for (const auto& cell : field_) {
            if (cell.first.row > row) {
                row = cell.first.row;
            }
            if (cell.first.col > col) {
                col = cell.first.col;
            }
        }
        return{ row + 1, col + 1  };
    }
    else {
        return { 0,0 };
    }
}

void Sheet::PrintValues(std::ostream& output) const {
    Size size = GetPrintableSize();
    int cols_count = size.cols;
    for (int row = 0; row < size.rows; ++row) {
        for (int col = 0; col < size.cols; ++col) {
            if (field_.count({ row,col }) > 0) {
                output << field_.at({ row, col })->GetValue();
            }
            if (cols_count > 1) {
                output << '\t';
                --cols_count;
            }
            else if (cols_count == 1) {
                output << '\n';
                cols_count = size.cols;
            }
        }
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    Size size = GetPrintableSize();
    int cols_count = size.cols;
    for (int row = 0; row < size.rows; ++row) {
        for (int col = 0; col < size.cols; ++col) {
            if (field_.count({ row,col }) > 0) {
                output << field_.at({ row, col })->GetText();
            }
            if (cols_count > 1) {
                output << '\t';
                --cols_count;
            }
            else if (cols_count == 1) {
                output << '\n';
                cols_count = size.cols;
            }
        }
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}