// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 desvaters
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <launch/LaunchStep.h>

/**
 * Writes the launcher's keybind profile into the options.txt of an instance on its first launch.
 *
 * Instance creation only marks the instance, because the format depends on the Minecraft version's data version, which
 * is read from the client jar and therefore only known once the game files are downloaded.
 */
class ApplyKeybindProfile : public LaunchStep {
    Q_OBJECT
   public:
    explicit ApplyKeybindProfile(LaunchTask* parent) : LaunchStep(parent) {}
    ~ApplyKeybindProfile() override = default;

    void executeTask() override;
    bool canAbort() const override { return false; }
};
