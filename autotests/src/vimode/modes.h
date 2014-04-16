/*
 * This file is part of the KDE libraries
 *
 * Copyright (C) 2014 Miquel Sabaté Solà <mikisabate@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */


#ifndef MODES_TEST_H
#define MODES_TEST_H


#include "base.h"


class ModesTest : public BaseTest
{
    Q_OBJECT

private Q_SLOTS:
    // Normal mode.
    void NormalModeMotionsTest();
    void NormalModeCommandsTest();
    void NormalModeControlTest();
    // TODO: this one is ugly
    void NormalModeNotYetImplementedFeaturesTest();

    // Insert mode.
    void InsertModeTests();

    // Visual mode.
    void VisualModeTests();

    // Command mode.
    void CommandModeTests();

    // Replace mode.
    void ReplaceModeTest();
};


#endif /* MODES_TEST_H */
