#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "presentation/gui_script.hpp"

TEST(GuiScriptTest, SplitsStepsAndKeepsQuotedWordsWhole) {
  const auto steps = ParseGuiScript(
      "# a comment\n"
      "\n"
      "tab Categories\n"
      "choose \"Assign category:\" UPK\r\n");

  ASSERT_TRUE(steps.has_value());
  ASSERT_EQ(steps->size(), 2U);
  EXPECT_EQ((*steps)[0].line, 3);
  EXPECT_EQ((*steps)[0].command, "tab");
  EXPECT_EQ((*steps)[1].line, 4);
  EXPECT_EQ((*steps)[1].arguments,
            (std::vector<std::string>{"Assign category:", "UPK"}));
}

TEST(GuiScriptTest, AnUnknownCommandFailsTheWholeScriptWithItsLine) {
  const auto steps = ParseGuiScript("tab Entries\nclik \"Add Row\"\n");

  ASSERT_FALSE(steps.has_value());
  EXPECT_EQ(steps.error().line, 2);
  EXPECT_EQ(steps.error().message, "unknown command: clik");
}

TEST(GuiScriptTest, AWrongArgumentCountFails) {
  EXPECT_FALSE(ParseGuiScript("edit-cell 1 Name\n").has_value());
  EXPECT_FALSE(ParseGuiScript("quit now\n").has_value());
}

TEST(GuiScriptTest, AnUnclosedQuoteFails) {
  EXPECT_FALSE(ParseGuiScript("click \"Add Row\n").has_value());
}

TEST(GuiScriptTest, RowListsExpandRanges) {
  const auto rows = ParseRowList({"1", "3-5", "9"});

  ASSERT_TRUE(rows.has_value());
  EXPECT_EQ(*rows, (std::vector<int>{1, 3, 4, 5, 9}));
}

TEST(GuiScriptTest, RowListsCountFromOne) {
  EXPECT_FALSE(ParseRowList({"0"}).has_value());
  EXPECT_FALSE(ParseRowList({"5-3"}).has_value());
  EXPECT_FALSE(ParseRowList({"x"}).has_value());
}
