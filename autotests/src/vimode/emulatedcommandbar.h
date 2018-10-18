/* This file is part of the KDE libraries

   Copyright (C) 2011 Kuzmich Svyatoslav
   Copyright (C) 2012 - 2013 Simon St James <kdedevel@etotheipiplusone.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef EMULATEDCOMMANDBAR_TEST_H
#define EMULATEDCOMMANDBAR_TEST_H

#include "base.h"

class QCompleter;
class QLabel;
class QColor;

class EmulatedCommandBarTest : public BaseTest
{
  Q_OBJECT

private Q_SLOTS:
  void EmulatedCommandBarTests();

private:
  QCompleter *emulatedCommandBarCompleter();

  void verifyCommandBarCompletionVisible();
  void verifyCommandBarCompletionsMatches(const QStringList& expectedCompletionList);
  void verifyCommandBarCompletionContains(const QStringList& expectedCompletionList);
  QLabel *emulatedCommandTypeIndicator();
  void verifyCursorAt(const KTextEditor::Cursor& expectedCursorPos);

  void clearSearchHistory();
  QStringList searchHistory();
  void clearCommandHistory();
  QStringList commandHistory();
  void clearReplaceHistory();
  QStringList replaceHistory();

  QList<Kate::TextRange *> rangesOnFirstLine();
  void verifyTextEditBackgroundColour(const QColor& expectedBackgroundColour);
  QLabel* commandResponseMessageDisplay();
  void waitForEmulatedCommandBarToHide(long int timeout);
  void verifyShowsNumberOfReplacementsAcrossNumberOfLines(int numReplacements, int acrossNumLines);
};

class FailsIfSlotNotCalled : public QObject
{
  Q_OBJECT
public:
  FailsIfSlotNotCalled();
  ~FailsIfSlotNotCalled();
public Q_SLOTS:
  void slot();
private:
  bool m_slotWasCalled = false;
};

class FailsIfSlotCalled : public QObject
{
  Q_OBJECT
public:
  explicit FailsIfSlotCalled(const QString& failureMessage);
public Q_SLOTS:
  void slot();
private:
  const QString m_failureMessage;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;


