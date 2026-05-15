#pragma once
#include "Table.hpp"
#include <sstream>
#include <iomanip>
#include <vector>
#include <string>

class ResultFormatter {
public:
    static std::string serialize(const ResultSet& rs) {
        std::ostringstream oss;
        oss << "OK\n";
        oss << rs.columns.size() << "\n";
        for (auto& c : rs.columns) oss << c << "\n";
        oss << rs.rows.size() << "\n";
        for (auto& row : rs.rows) {
            for (auto& col : rs.columns) {
                const Value* v = row.get(col);
                if (v) {
                    oss << (v->type==Long ? "L" : "T") << "\n";
                    oss << v->toString() << "\n";
                } else {
                    oss << "T\n\n";
                }
            }
        }
        oss << rs.message << "\n";
        oss << rs.affected << "\n";
        return oss.str();
    }

    static std::string serializeError(const std::string& msg) {
        return "ERR\n" + msg + "\n";
    }

    static bool deserialize(const std::string& payload,
                            ResultSet& out, std::string& errMsg) {
        std::istringstream iss(payload);
        std::string status;
        std::getline(iss, status);
        if (status == "ERR") {
            std::getline(iss, errMsg);
            return false;
        }
        // columns
        int ncols = 0; iss >> ncols; iss.ignore();
        for (int i=0;i<ncols;i++) {
            std::string c; std::getline(iss,c);
            out.columns.push_back(c);
        }
        int nrows = 0; iss >> nrows; iss.ignore();
        for (int r=0;r<nrows;r++) {
            Row row;
            for (int c=0;c<ncols;c++) {
                std::string typeTag, valStr;
                std::getline(iss,typeTag);
                std::getline(iss,valStr);
                Value v;
                if (typeTag=="L") { v.type=Long; v.longVal=std::stol(valStr.empty()?"0":valStr); }
                else               { v.type=Text; v.textVal=valStr; }
                row.set(out.columns[c], v);
            }
            out.rows.push_back(row);
        }
        std::getline(iss, out.message);
        iss >> out.affected;
        return true;
    }

    // Pretty-print a ResultSet as an ASCII table
    static std::string prettyPrint(const ResultSet& rs) {
        if (rs.columns.empty()) {
            return rs.message + "\n";
        }
        // compute column widths
        std::vector<size_t> widths(rs.columns.size());
        for (size_t i=0;i<rs.columns.size();i++) widths[i]=rs.columns[i].size();
        for (auto& row : rs.rows) {
            for (size_t i=0;i<rs.columns.size();i++) {
                const Value* v = row.get(rs.columns[i]);
                if (v) widths[i]=std::max(widths[i], v->toString().size());
            }
        }

        std::ostringstream oss;
        // separator
        auto sep = [&](){
            oss << "+";
            for (size_t i=0;i<rs.columns.size();i++) oss << std::string(widths[i]+2,'-') << "+";
            oss << "\n";
        };
        // header
        sep();
        oss << "|";
        for (size_t i=0;i<rs.columns.size();i++)
            oss << " " << std::left << std::setw((int)widths[i]) << rs.columns[i] << " |";
        oss << "\n";
        sep();
        // rows
        for (auto& row : rs.rows) {
            oss << "|";
            for (size_t i=0;i<rs.columns.size();i++) {
                const Value* v = row.get(rs.columns[i]);
                std::string s = v ? v->toString() : "";
                oss << " " << std::left << std::setw((int)widths[i]) << s << " |";
            }
            oss << "\n";
        }
        sep();
        oss << rs.message << "\n";
        return oss.str();
    }
};
