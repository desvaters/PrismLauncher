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

#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>
#include <Qt>

class QKeyEvent;

/**
 * A launcher-wide set of Minecraft keybinds that is written into the options.txt of newly created instances.
 *
 * Keys are stored in the format of current Minecraft versions ("key.keyboard.r", "key.mouse.left") and translated to the
 * format of the instance's Minecraft version when the options.txt is written.
 */
namespace KeybindProfile {

/// The input library a Minecraft version uses, which decides how keys are stored in options.txt.
enum class InputBackend {
    Lwjgl2,  ///< before 1.13: LWJGL2 key codes as integers
    Glfw,    ///< 1.13 up to the SDL switch: GLFW key names
    Sdl,     ///< SDL scancode names
};

struct Action {
    QString id;        ///< e.g. "key.togglePerspective"
    QString category;  ///< e.g. "key.categories.misc"
    QString defaultKey;
};

/// Action id -> key name.
using Profile = QMap<QString, QString>;

const QList<Action>& vanillaActions();
QString actionDisplayName(const QString& actionId);
QString categoryDisplayName(const QString& category);
QString keyDisplayName(const QString& key);

/// Reads the profile from the global "KeybindProfile" setting.
Profile load();
void save(const Profile& profile);

/// Returns the key name for a key press, or an empty string if the key cannot be expressed in options.txt.
QString keyFromEvent(const QKeyEvent* event);
QString keyFromMouseButton(Qt::MouseButton button);

/**
 * Builds the content of an options.txt that only contains the keybinds of the profile.
 *
 * Minecraft fills every missing option with its default. Bindings that cannot be expressed for the given backend are
 * listed in `skipped`.
 *
 * Without a matching `version:` line, Minecraft's data fixer parses every keybind as an LWJGL2 integer and throws on
 * key names, which discards all options. Therefore `dataVersion` must be the data version of the instance's Minecraft
 * version (or lower, but not below 1446) for GLFW and SDL; it is ignored for LWJGL2.
 */
QString buildOptions(const Profile& profile, InputBackend backend, int dataVersion, QStringList* skipped = nullptr);

}  // namespace KeybindProfile
