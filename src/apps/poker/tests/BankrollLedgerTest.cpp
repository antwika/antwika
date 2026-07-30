#include <gtest/gtest.h>

#include <map>
#include <string>

#include <antwika/holdem/Chips.hpp>

#include "antwika/poker/BankrollError.hpp"
#include "antwika/poker/BankrollLedger.hpp"

using antwika::holdem::Chips;
using antwika::poker::BankrollError;
using antwika::poker::BankrollLedger;

TEST(BankrollLedgerTest, BalanceOf_ReportsNothingForAnUnknownPlayer)
{
    const BankrollLedger ledger;

    EXPECT_EQ(ledger.balanceOf("nobody"), 0U);
    EXPECT_TRUE(ledger.balances().empty());
}

TEST(BankrollLedgerTest, Deposit_OpensAnAccountOnFirstUse)
{
    BankrollLedger ledger;
    ledger.deposit("alice", 500);

    EXPECT_EQ(ledger.balanceOf("alice"), 500U);
}

TEST(BankrollLedgerTest, Deposit_AddsToAnExistingBalance)
{
    BankrollLedger ledger;
    ledger.deposit("alice", 500);
    ledger.deposit("alice", 250);

    EXPECT_EQ(ledger.balanceOf("alice"), 750U);
}

TEST(BankrollLedgerTest, Withdraw_TakesChipsOutOfTheBalance)
{
    BankrollLedger ledger;
    ledger.deposit("alice", 500);
    ledger.withdraw("alice", 200);

    EXPECT_EQ(ledger.balanceOf("alice"), 300U);
}

TEST(BankrollLedgerTest, Withdraw_AllowsEmptyingTheBalanceExactly)
{
    BankrollLedger ledger;
    ledger.deposit("alice", 500);
    ledger.withdraw("alice", 500);

    EXPECT_EQ(ledger.balanceOf("alice"), 0U);
}

// This is the rule the ledger exists for.
// No player may ever put more at risk than they actually hold.
TEST(BankrollLedgerTest, Withdraw_RefusesMoreThanTheBalanceHolds)
{
    BankrollLedger ledger;
    ledger.deposit("alice", 500);

    EXPECT_THROW(ledger.withdraw("alice", 501), BankrollError);
    EXPECT_EQ(ledger.balanceOf("alice"), 500U);
}

TEST(BankrollLedgerTest, Withdraw_RefusesAnUnknownPlayer)
{
    BankrollLedger ledger;

    EXPECT_THROW(ledger.withdraw("nobody", 1), BankrollError);
}

TEST(BankrollLedgerTest, Balances_ReportsEveryAccountInNameOrder)
{
    BankrollLedger ledger;
    ledger.deposit("carol", 300);
    ledger.deposit("alice", 100);
    ledger.deposit("bob", 200);

    const std::map<std::string, Chips> expected{
        {"alice", 100},
        {"bob", 200},
        {"carol", 300},
    };
    EXPECT_EQ(ledger.balances(), expected);
}
