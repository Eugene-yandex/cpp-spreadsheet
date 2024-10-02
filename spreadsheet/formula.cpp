#include "formula.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>
#include <string>

using namespace std::literals;

namespace {
    double GetDoubleFromTextImpl(const std::string& str) {
        for (const char ch : str) {
            if (!std::isdigit(ch)) {
                throw FormulaError(FormulaError::Category::Value);
            }
        }
        return std::stod(str);
    }
class Formula : public FormulaInterface {
public:
// Реализуйте следующие методы:
    explicit Formula(std::string expression) try :
        ast_(ParseFormulaAST(expression)) {
    }
    catch (const FormulaException& exc) {
        throw FormulaException("error sintacsis");
    }
    Value Evaluate(const SheetInterface& sheet) const override {
        try {
            std::function<double(Position)> func_get_value_cell = [&](const Position pos) {
                if (pos.row > Position::MAX_ROWS || pos.col > Position::MAX_COLS) {
                    throw FormulaError(FormulaError::Category::Ref);
                }
                if (sheet.GetCell(pos) == nullptr) { //IsValid() проверяется здесь, дополнительная проверка в методе будет лишней
                    return 0.0;
                }

                auto value_cell = sheet.GetCell(pos)->GetValue();
                if (std::holds_alternative<std::string>(value_cell)) {
                    std::string str_value(std::get<std::string>(value_cell));
                    if (str_value.length() == 0) {
                        return 0.0;
                    }
                    else {
                        return GetDoubleFromTextImpl(str_value);
                    }
                }
                else if(std::holds_alternative<double>(value_cell)){
                    return  std::get<double>(value_cell);
                }
                else {
                    throw FormulaError(FormulaError::Category::Arithmetic);
                }
            };

            return ast_.Execute(func_get_value_cell);
        }
        catch (const FormulaError& er) {
            return er;
        }
    }
    std::string GetExpression() const override {
        std::ostringstream out;
        ast_.PrintFormula(out);
        return out.str();
    }

    std::vector<Position> GetReferencedCells() const override {
        auto& cells = ast_.GetDependentCells();
        std::vector<Position> referenced_cells{ cells.begin(),cells.end() };
        referenced_cells.erase(std::unique(referenced_cells.begin(), referenced_cells.end()), referenced_cells.end());
        return referenced_cells;
    }
private:
    FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));

}