/*
    SPDX-FileCopyrightText: 2010 Christoph Cullmann <cullmann@kde.org>

    Based on code of the SmartCursor/Range by:
    SPDX-FileCopyrightText: 2003-2005 Hamish Rodda <rodda@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "movingrangefeedback.h"

using namespace KTextEditor;

MovingRangeFeedback::MovingRangeFeedback() = default;

MovingRangeFeedback::~MovingRangeFeedback() = default;

void MovingRangeFeedback::rangeEmpty(MovingRange *)
{
}

void MovingRangeFeedback::rangeInvalid(MovingRange *)
{
}

void MovingRangeFeedback::mouseEnteredRange(MovingRange *, View *)
{
}

void MovingRangeFeedback::mouseExitedRange(MovingRange *, View *)
{
}

void MovingRangeFeedback::caretEnteredRange(MovingRange *, View *)
{
}

void MovingRangeFeedback::caretExitedRange(MovingRange *, View *)
{
}
