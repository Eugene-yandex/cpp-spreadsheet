#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <unordered_map>

class Cell;

class Sheet : public SheetInterface {
public:
    Sheet();
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    //Cell* TakeCell(Position pos); //добавление данных методов поможет избавиться от dynamic_cast. Но добавление этих методов
    //const Cell* TakeCell(Position pos) const; // покажет ненужность метода GetCell. А GetCell используется в родительском классе.

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;


private:
    std::unordered_map<Position,std::unique_ptr<Cell>,PositionHasher> field_;
};