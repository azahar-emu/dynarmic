/* This file is part of the dynarmic project.
 * Copyright (c) 2026 Superhac
 * SPDX-License-Identifier: 0BSD
 */

#include <string>
#include <variant>

#include <catch2/catch_test_macros.hpp>

#include "dynarmic/common/variant_util.h"
#include "dynarmic/ir/terminal.h"

using namespace Dynarmic;

namespace {

// Names a terminal's shape, looking inside the recursive ones.
std::string Shape(const IR::Terminal& terminal) {
    return Common::VisitVariant<std::string>(terminal, [](const auto& t) -> std::string {
        using T = std::decay_t<decltype(t)>;
        if constexpr (std::is_same_v<T, IR::Term::If>) {
            return "If(" + Shape(t.then_) + "," + Shape(t.else_) + ")";
        } else if constexpr (std::is_same_v<T, IR::Term::CheckBit>) {
            return "CheckBit(" + Shape(t.then_) + "," + Shape(t.else_) + ")";
        } else if constexpr (std::is_same_v<T, IR::Term::CheckHalt>) {
            return "CheckHalt(" + Shape(t.else_) + ")";
        } else if constexpr (std::is_same_v<T, IR::Term::LinkBlock>) {
            return "LinkBlock";
        } else if constexpr (std::is_same_v<T, IR::Term::ReturnToDispatch>) {
            return "ReturnToDispatch";
        } else if constexpr (std::is_same_v<T, IR::Term::Interpret>) {
            return "Interpret";
        } else {
            return "other";
        }
    });
}

}  // namespace

TEST_CASE("Terminal: default is Invalid", "[ir]") {
    IR::Terminal terminal;
    REQUIRE(terminal.index() == 0);
    REQUIRE(std::holds_alternative<IR::Term::Invalid>(terminal));
}

TEST_CASE("Terminal: nested terminals visit, copy and assign", "[ir]") {
    const IR::LocationDescriptor a{0x100}, b{0x200};
    IR::Terminal terminal = IR::Term::If{IR::Cond::EQ, IR::Term::LinkBlock{a}, IR::Term::CheckHalt{IR::Term::CheckBit{IR::Term::ReturnToDispatch{}, IR::Term::LinkBlock{b}}}};
    REQUIRE(Shape(terminal) == "If(LinkBlock,CheckHalt(CheckBit(ReturnToDispatch,LinkBlock)))");

    IR::Terminal copy = terminal;
    terminal = IR::Term::Interpret{a};  // the copy is independent
    REQUIRE(Shape(terminal) == "Interpret");
    REQUIRE(Shape(copy) == "If(LinkBlock,CheckHalt(CheckBit(ReturnToDispatch,LinkBlock)))");

    IR::Terminal moved = std::move(copy);
    REQUIRE(Shape(moved) == "If(LinkBlock,CheckHalt(CheckBit(ReturnToDispatch,LinkBlock)))");

    copy = moved;  // assigning into a moved-from terminal
    REQUIRE(Shape(copy) == Shape(moved));
}

TEST_CASE("Terminal: get_if on a non-recursive alternative", "[ir]") {
    const IR::LocationDescriptor a{0x1234};
    IR::Terminal terminal = IR::Term::Interpret{a};
    const auto* interpret = std::get_if<IR::Term::Interpret>(&terminal);
    REQUIRE(interpret != nullptr);
    REQUIRE(interpret->next == a);
    REQUIRE(std::get_if<IR::Term::LinkBlock>(&terminal) == nullptr);
}
