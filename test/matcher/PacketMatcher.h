/*
 * PLUG - software to operate Fender Mustang amplifier
 *        Linux replacement for Fender FUSE software
 *
 * Copyright (C) 2017-2020  offa
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
 */

#pragma once

#include "com/Packet.h"
#include "com/MustangConstants.h"
#include "data_structs.h"
#include <tuple>
#include <gmock/gmock.h>


namespace test::matcher
{
    MATCHER_P4(AmpDataIs, ampId, v0, v1, v2, "")
    {
        const std::tuple actual{arg[plug::com::v1::AMPLIFIER], arg[40], arg[43], arg[44], arg[45], arg[46], arg[50], arg[54]};
        const auto [a0, a1, a2, a3, a4, a5, a6, a7] = actual;
        *result_listener << " with amp specific values: ("
                         << int{a0} << ", {" << int{a1} << ", " << int{a2} << "}, {"
                         << int{a3} << ", " << int{a4} << ", " << int{a5} << ", " << int{a6}
                         << "}, " << int{a7} << ")";
        return std::tuple{ampId, v0, v0, v1, v1, v1, v1, v2} == actual;
    }

    MATCHER_P(CabinetDataIs, cabinetValue, "")
    {
        const auto actual = arg[plug::com::v1::CABINET];
        *result_listener << " with cabinet data: " << int{actual};
        return actual == cabinetValue;
    }

    MATCHER_P4(EffectDataIs, dsp, effect, v0, v1, "")
    {
        const std::tuple actual{arg[plug::com::v1::DSP], arg[plug::com::v1::EFFECT], arg[19], arg[20]};
        const auto [a0, a1, a2, a3] = actual;
        *result_listener << " with effect values: (" << int{a0} << ", " << int{a1}
                         << ", " << int{a2} << ", " << int{a3} << ")";

        return std::tuple{dsp, effect, v0, v1} == actual;
    }

    MATCHER_P6(KnobsAre, k1, k2, k3, k4, k5, k6, "")
    {
        const std::tuple actual{arg[plug::com::v1::KNOB1], arg[plug::com::v1::KNOB2], arg[plug::com::v1::KNOB3],
                                arg[plug::com::v1::KNOB4], arg[plug::com::v1::KNOB5], arg[plug::com::v1::KNOB6]};
        const auto [a1, a2, a3, a4, a5, a6] = actual;
        *result_listener << " with knobs: (" << int{a1} << ", " << int{a2} << ", " << int{a3}
                         << ", " << int{a4} << ", " << int{a5} << ", " << int{a6} << ")";
        return std::tuple{k1, k2, k3, k4, k5, k6} == actual;
    }

    MATCHER_P(FxKnobIs, value, "")
    {
        const auto a = arg[plug::com::v1::FXKNOB];
        *result_listener << " with FX Knob " << int{a};
        return value == a;
    }
}
