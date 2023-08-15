/*****************************************************************************
 * Alpine Terrain Renderer
 * Copyright (C) 2022 Adam Celarek
 * Copyright (C) 2023 Jakob Lindner
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *****************************************************************************/

#pragma once

#include "InteractionStyle.h"
#include "nucleus/utils/Stopwatch.h"

namespace nucleus::camera {
class LinearCameraAnimation : public InteractionStyle
{
    utils::Stopwatch m_stopwatch = {};
    glm::dmat4 m_start;
    glm::dmat4 m_end;

    int m_total_duration = 10'000;
    float m_current_duration = 0;

public:
    LinearCameraAnimation(Definition start, Definition end);
    void reset_interaction(Definition camera, AbstractDepthTester* depth_tester) override;
    std::optional<Definition> update(Definition camera, AbstractDepthTester* depth_tester) override;
    std::optional<glm::vec2> get_operation_centre() override;
private:
    float ease_in_out(float t);
    glm::vec2 m_operation_centre_screen = {};
};
}