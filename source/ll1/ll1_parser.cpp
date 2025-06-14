#include <algorithm>
#include <cstddef>
#include <iostream>
#include <span>
#include <string>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

#include "grammar.hpp"
#include "ll1_parser.hpp"
#include "symbol_table.hpp"

LL1Parser::LL1Parser(Grammar gr) : gr_(std::move(gr)) {
    ComputeFirstSets();
    ComputeFollowSets();
}

bool LL1Parser::CreateLL1Table() {
    if (first_sets_.empty() || follow_sets_.empty()) {
        ComputeFirstSets();
        ComputeFollowSets();
    }
    size_t nrows{gr_.g_.size()};
    ll1_t_.reserve(nrows);
    bool has_conflict{false};
    for (const auto& [lhs, productions] : gr_.g_) {
        std::unordered_map<std::string, std::vector<production>> column;
        for (const production& p : productions) {
            std::unordered_set<std::string> ds = PredictionSymbols(lhs, p);
            column.reserve(ds.size());
            for (const std::string& symbol : ds) {
                auto& cell = column[symbol];
                if (!cell.empty()) {
                    has_conflict = true;
                }
                cell.push_back(p);
            }
        }
        ll1_t_.try_emplace(lhs, std::move(column));
    }
    return !has_conflict;
}

void LL1Parser::First(std::span<const std::string>     rule,
                      std::unordered_set<std::string>& result) {
    if (rule.empty() || (rule.size() == 1 && rule[0] == gr_.st_.EPSILON_)) {
        result.insert(gr_.st_.EPSILON_);
        return;
    }

    if (rule.size() > 1 && rule[0] == gr_.st_.EPSILON_) {
        First(std::span<const std::string>(rule.begin() + 1, rule.end()),
              result);
    } else {

        if (gr_.st_.IsTerminal(rule[0])) {
            // EOL cannot be in first sets, if we reach EOL it means that the
            // axiom is nullable, so epsilon is included instead
            if (rule[0] == gr_.st_.EOL_) {
                result.insert(gr_.st_.EPSILON_);
                return;
            }
            result.insert(rule[0]);
            return;
        }

        const std::unordered_set<std::string>& fii = first_sets_[rule[0]];
        for (const auto& s : fii) {
            if (s != gr_.st_.EPSILON_) {
                result.insert(s);
            }
        }

        if (!fii.contains(gr_.st_.EPSILON_)) {
            return;
        }
        First(std::span<const std::string>(rule.begin() + 1, rule.end()),
              result);
    }
}

// Least fixed point
void LL1Parser::ComputeFirstSets() {
    // Init all FIRST to empty
    for (const auto& [nonTerminal, _] : gr_.g_) {
        first_sets_[nonTerminal] = {};
    }

    bool changed;
    do {
        auto old_first_sets = first_sets_; // Copy current state

        for (const auto& [nonTerminal, productions] : gr_.g_) {
            for (const auto& prod : productions) {
                std::unordered_set<std::string> tempFirst;
                First(prod, tempFirst);

                if (tempFirst.contains(gr_.st_.EOL_)) {
                    tempFirst.erase(gr_.st_.EOL_);
                    tempFirst.insert(gr_.st_.EPSILON_);
                }

                auto& current_set = first_sets_[nonTerminal];
                current_set.insert(tempFirst.begin(), tempFirst.end());
            }
        }

        // Until all remain the same
        changed = (old_first_sets != first_sets_);

    } while (changed);
}

void LL1Parser::ComputeFollowSets() {
    for (const auto& [nt, _] : gr_.g_) {
        follow_sets_[nt] = {};
    }
    follow_sets_[gr_.axiom_].insert(gr_.st_.EOL_);

    bool changed;
    do {
        changed = false;
        for (const auto& [lhs, productions] : gr_.g_) {
            for (const production& rhs : productions) {
                for (size_t i = 0; i < rhs.size(); ++i) {
                    const std::string& symbol = rhs[i];
                    if (!gr_.st_.IsTerminal(symbol)) {
                        changed |= UpdateFollow(symbol, lhs, rhs, i);
                    }
                }
            }
        }
    } while (changed);
}

bool LL1Parser::UpdateFollow(const std::string& symbol, const std::string& lhs,
                             const production& rhs, size_t i) {
    bool changed = false;

    std::unordered_set<std::string> first_remaining;
    if (i + 1 < rhs.size()) {
        First(std::span<const std::string>(rhs.begin() + i + 1, rhs.end()),
              first_remaining);
    } else {
        first_remaining.insert(gr_.st_.EPSILON_);
    }

    // Add FIRST(β) \ {ε}
    for (const auto& terminal : first_remaining) {
        if (terminal != gr_.st_.EPSILON_) {
            changed |= follow_sets_[symbol].insert(terminal).second;
        }
    }

    // If FIRST(β) contains ε, add FOLLOW(lhs)
    if (first_remaining.contains(gr_.st_.EPSILON_)) {
        for (const auto& terminal : follow_sets_[lhs]) {
            changed |= follow_sets_[symbol].insert(terminal).second;
        }
    }

    return changed;
}

std::unordered_set<std::string> LL1Parser::Follow(const std::string& arg) {
    if (!follow_sets_.contains(arg)) {
        return {};
    }
    return follow_sets_.at(arg);
}

std::unordered_set<std::string>
LL1Parser::PredictionSymbols(const std::string&              antecedent,
                             const std::vector<std::string>& consequent) {
    std::unordered_set<std::string> hd{};
    First(consequent, hd);
    if (!hd.contains(gr_.st_.EPSILON_)) {
        return hd;
    }
    hd.erase(gr_.st_.EPSILON_);
    hd.merge(Follow(antecedent));
    return hd;
}

std::string LL1Parser::PrintTable() {
    std::ostringstream oss;
    oss << "\nLL(1) Table:\n";

    std::unordered_set<std::string> all_terminals;
    for (const auto& [lhs, column] : ll1_t_) {
        for (const auto& [terminal, _] : column) {
            all_terminals.insert(terminal);
        }
    }

    std::vector<std::string> non_terminals;
    for (const auto& [lhs, _] : ll1_t_) {
        non_terminals.push_back(lhs);
    }

    std::ranges::sort(non_terminals,
        [this](const std::string& a, const std::string& b) {
            if (a == gr_.axiom_) return true;
            if (b == gr_.axiom_) return false;
            return a < b;
        });

    for (const auto& lhs : non_terminals) {
        oss << "\n" << lhs << ":\n";

        for (const auto& terminal : all_terminals) {
            oss << "  " << terminal << " -> ";

            auto innerIt = ll1_t_.at(lhs).find(terminal);
            if (innerIt != ll1_t_.at(lhs).end()) {
                const auto& prods = innerIt->second;
                for (const auto& prod : prods) {
                    oss << "[";
                    for (const auto& smb : prod) {
                        oss << smb << " ";
                    }
                    oss << "]";
                }
            } else {
                oss << "-";
            }
            oss << "\n";
        }
    }
    return oss.str();
}