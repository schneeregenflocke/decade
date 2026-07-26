#include <gtest/gtest.h>

#include "domain/text_edit_buffer.hpp"

using Direction = TextEditBuffer::Direction;
using Selection = TextEditBuffer::Selection;

TEST(TextEditBufferTest, ConstructionPutsCaretAtEndWithoutSelection) {
  const TextEditBuffer buffer(U"Titel");
  EXPECT_EQ(buffer.Caret(), 5U);
  EXPECT_FALSE(buffer.HasSelection());
}

TEST(TextEditBufferTest, InsertAtCaretGrowsTextAndAdvancesCaret) {
  TextEditBuffer buffer(U"Tiel");
  buffer.SetCaret(2, Selection::kReplace);
  buffer.Insert(U't');
  EXPECT_EQ(buffer.Text(), U"Titel");
  EXPECT_EQ(buffer.Caret(), 3U);
}

TEST(TextEditBufferTest, UmlautCountsAsOneStep) {
  TextEditBuffer buffer(U"Uber");
  buffer.SetCaret(0, Selection::kReplace);
  buffer.DeleteAfter();
  buffer.Insert(U'Ü');
  EXPECT_EQ(buffer.Text(), U"Über");
  EXPECT_EQ(buffer.Caret(), 1U);
}

TEST(TextEditBufferTest, InsertReplacesTheSelection) {
  TextEditBuffer buffer(U"altes Wort");
  buffer.SetCaret(0, Selection::kReplace);
  buffer.SetCaret(5, Selection::kExtend);
  buffer.Insert(U"neues");
  EXPECT_EQ(buffer.Text(), U"neues Wort");
  EXPECT_FALSE(buffer.HasSelection());
}

TEST(TextEditBufferTest, DeleteBeforeRemovesCharacterLeftOfCaret) {
  TextEditBuffer buffer(U"Titel");
  buffer.DeleteBefore();
  EXPECT_EQ(buffer.Text(), U"Tite");
  EXPECT_EQ(buffer.Caret(), 4U);
}

TEST(TextEditBufferTest, DeleteBeforeAtStartDoesNothing) {
  TextEditBuffer buffer(U"Titel");
  buffer.SetCaret(0, Selection::kReplace);
  buffer.DeleteBefore();
  EXPECT_EQ(buffer.Text(), U"Titel");
  EXPECT_EQ(buffer.Caret(), 0U);
}

TEST(TextEditBufferTest, DeleteAfterAtEndDoesNothing) {
  TextEditBuffer buffer(U"Titel");
  buffer.DeleteAfter();
  EXPECT_EQ(buffer.Text(), U"Titel");
}

TEST(TextEditBufferTest, DeleteRemovesTheSelectionRegardlessOfDirection) {
  TextEditBuffer before(U"Titelzeile");
  before.SetCaret(5, Selection::kReplace);
  before.SetCaret(10, Selection::kExtend);
  before.DeleteBefore();
  EXPECT_EQ(before.Text(), U"Titel");

  TextEditBuffer after(U"Titelzeile");
  after.SetCaret(5, Selection::kReplace);
  after.SetCaret(10, Selection::kExtend);
  after.DeleteAfter();
  EXPECT_EQ(after.Text(), U"Titel");
}

TEST(TextEditBufferTest, SelectionSpansFromAnchorToCaretInBothDirections) {
  TextEditBuffer buffer(U"Titelzeile");
  buffer.SetCaret(10, Selection::kReplace);
  buffer.SetCaret(5, Selection::kExtend);
  EXPECT_EQ(buffer.SelectionBegin(), 5U);
  EXPECT_EQ(buffer.SelectionEnd(), 10U);
  EXPECT_EQ(buffer.SelectedText(), U"zeile");
}

TEST(TextEditBufferTest, MoveWithoutShiftCollapsesSelectionToItsEdge) {
  TextEditBuffer buffer(U"Titelzeile");
  buffer.SetCaret(5, Selection::kReplace);
  buffer.SetCaret(10, Selection::kExtend);
  buffer.MoveCaret(Direction::kLeft, Selection::kReplace);
  EXPECT_FALSE(buffer.HasSelection());
  EXPECT_EQ(buffer.Caret(), 5U);
}

TEST(TextEditBufferTest, MoveWithShiftExtendsTheSelection) {
  TextEditBuffer buffer(U"Titel");
  buffer.SetCaret(5, Selection::kReplace);
  buffer.MoveCaret(Direction::kLeft, Selection::kExtend);
  buffer.MoveCaret(Direction::kLeft, Selection::kExtend);
  EXPECT_EQ(buffer.SelectedText(), U"el");
}

TEST(TextEditBufferTest, MoveStopsAtBothEnds) {
  TextEditBuffer buffer(U"Ti");
  buffer.SetCaret(0, Selection::kReplace);
  buffer.MoveCaret(Direction::kLeft, Selection::kReplace);
  EXPECT_EQ(buffer.Caret(), 0U);
  buffer.MoveCaret(Direction::kEnd, Selection::kReplace);
  buffer.MoveCaret(Direction::kRight, Selection::kReplace);
  EXPECT_EQ(buffer.Caret(), 2U);
}

TEST(TextEditBufferTest, BeginAndEndJumpToTheEdges) {
  TextEditBuffer buffer(U"Titel");
  buffer.MoveCaret(Direction::kBegin, Selection::kReplace);
  EXPECT_EQ(buffer.Caret(), 0U);
  buffer.MoveCaret(Direction::kEnd, Selection::kReplace);
  EXPECT_EQ(buffer.Caret(), 5U);
}

TEST(TextEditBufferTest, SelectAllCoversTheWholeText) {
  TextEditBuffer buffer(U"Titel");
  buffer.SelectAll();
  EXPECT_EQ(buffer.SelectedText(), U"Titel");
}

TEST(TextEditBufferTest, SelectWordPicksTheWordUnderTheIndex) {
  TextEditBuffer buffer(U"erstes zweites drittes");
  buffer.SelectWordAt(9);
  EXPECT_EQ(buffer.SelectedText(), U"zweites");
}

TEST(TextEditBufferTest, SelectWordOnSpacePicksTheGap) {
  TextEditBuffer buffer(U"erstes  zweites");
  buffer.SelectWordAt(6);
  EXPECT_EQ(buffer.SelectedText(), U"  ");
}

TEST(TextEditBufferTest, SelectWordOnEmptyTextKeepsCaret) {
  TextEditBuffer buffer;
  buffer.SelectWordAt(3);
  EXPECT_FALSE(buffer.HasSelection());
  EXPECT_EQ(buffer.Caret(), 0U);
}

TEST(TextEditBufferTest, SetCaretClampsBeyondTheEnd) {
  TextEditBuffer buffer(U"Titel");
  buffer.SetCaret(99, Selection::kReplace);
  EXPECT_EQ(buffer.Caret(), 5U);
}
