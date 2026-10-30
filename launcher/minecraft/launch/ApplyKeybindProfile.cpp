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

#include "ApplyKeybindProfile.h"

#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>

#include "FileSystem.h"
#include "archive/ArchiveReader.h"
#include "launch/LaunchTask.h"
#include "minecraft/KeybindProfile.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"

namespace {

// 1.13 has no version.json in its jar yet; its data version is a safe choice for all of 1.13.x.
constexpr int fallbackGlfwDataVersion = 1519;

int readDataVersion(MinecraftInstance* instance)
{
    const auto mainJar = instance->getPackProfile()->getProfile()->getMainJar();
    if (!mainJar) {
        return 0;
    }
    QStringList jars, nativeJars, native32Jars, native64Jars;
    mainJar->getApplicableFiles(instance->runtimeContext(), jars, nativeJars, native32Jars, native64Jars, instance->getLocalLibraryPath());
    if (jars.isEmpty()) {
        return 0;
    }
    const auto versionJson = MMCZip::ArchiveReader(jars.first()).readFile("version.json");
    if (!versionJson) {
        return 0;
    }
    return QJsonDocument::fromJson(*versionJson).object().value("world_version").toInt();
}

KeybindProfile::InputBackend detectBackend(MinecraftInstance* instance)
{
    auto backend = KeybindProfile::InputBackend::Lwjgl2;
    for (const auto& library : instance->getPackProfile()->getProfile()->getLibraries()) {
        const auto artifact = library->artifactId();
        if (artifact == "lwjgl-sdl") {
            return KeybindProfile::InputBackend::Sdl;
        }
        if (artifact == "lwjgl-glfw") {
            backend = KeybindProfile::InputBackend::Glfw;
        }
    }
    return backend;
}

}  // namespace

void ApplyKeybindProfile::executeTask()
{
    auto instance = m_parent->instance();
    auto settings = instance->settings();
    if (!settings->get("ApplyKeybindProfile").toBool()) {
        emitSucceeded();
        return;
    }
    // only ever try once, so that a later deletion of options.txt doesn't bring the profile back
    settings->set("ApplyKeybindProfile", false);

    const auto optionsPath = FS::PathCombine(instance->gameRoot(), "options.txt");
    if (QFileInfo::exists(optionsPath)) {
        emit logLine(tr("options.txt already exists, not applying the keybind profile."), MessageLevel::Launcher);
        emitSucceeded();
        return;
    }
    const auto profile = KeybindProfile::load();
    if (profile.isEmpty()) {
        emitSucceeded();
        return;
    }

    const auto backend = detectBackend(instance);
    int dataVersion = 0;
    if (backend != KeybindProfile::InputBackend::Lwjgl2) {
        dataVersion = readDataVersion(instance);
        if (dataVersion == 0) {
            dataVersion = fallbackGlfwDataVersion;
        }
    }

    QStringList skipped;
    const auto content = KeybindProfile::buildOptions(profile, backend, dataVersion, &skipped);
    if (const auto result = FS::write(optionsPath, content.toUtf8()); !result) {
        // not worth failing the launch over
        emit logLine(tr("Couldn't write the keybind profile to options.txt: %1").arg(result.error()), MessageLevel::Error);
        emitSucceeded();
        return;
    }
    emit logLine(tr("Applied the keybind profile to options.txt."), MessageLevel::Launcher);
    if (!skipped.isEmpty()) {
        emit logLine(tr("These keybinds can't be expressed for this Minecraft version and were skipped: %1").arg(skipped.join(", ")),
                     MessageLevel::Warning);
    }
    emitSucceeded();
}
