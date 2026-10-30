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

#include "KeybindProfile.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <optional>

#include "Application.h"
#include "settings/SettingsObject.h"

namespace KeybindProfile {

namespace {

const QString keyboardPrefix = QStringLiteral("key.keyboard.");
const QString mousePrefix = QStringLiteral("key.mouse.");

// The data version that renamed "key.swapHands" to "key.swapOffhand" (1.16 development cycle).
constexpr int swapOffhandDataVersion = 2558;

/**
 * PC scancode set 1, with 0x80 added for extended (E0-prefixed) keys. These are exactly the key codes LWJGL2 stored in
 * options.txt before 1.13, so the same table serves key capture and writing options for old versions.
 * Names are those of current (SDL) Minecraft versions without the "key.keyboard." prefix.
 */
const QHash<int, QString>& set1Names()
{
    static const QHash<int, QString> names = {
        { 1, "escape" },
        { 2, "1" },
        { 3, "2" },
        { 4, "3" },
        { 5, "4" },
        { 6, "5" },
        { 7, "6" },
        { 8, "7" },
        { 9, "8" },
        { 10, "9" },
        { 11, "0" },
        { 12, "minus" },
        { 13, "equal" },
        { 14, "backspace" },
        { 15, "tab" },
        { 16, "q" },
        { 17, "w" },
        { 18, "e" },
        { 19, "r" },
        { 20, "t" },
        { 21, "y" },
        { 22, "u" },
        { 23, "i" },
        { 24, "o" },
        { 25, "p" },
        { 26, "left.bracket" },
        { 27, "right.bracket" },
        { 28, "enter" },
        { 29, "left.control" },
        { 30, "a" },
        { 31, "s" },
        { 32, "d" },
        { 33, "f" },
        { 34, "g" },
        { 35, "h" },
        { 36, "j" },
        { 37, "k" },
        { 38, "l" },
        { 39, "semicolon" },
        { 40, "apostrophe" },
        { 41, "grave.accent" },
        { 42, "left.shift" },
        { 43, "backslash" },
        { 44, "z" },
        { 45, "x" },
        { 46, "c" },
        { 47, "v" },
        { 48, "b" },
        { 49, "n" },
        { 50, "m" },
        { 51, "comma" },
        { 52, "period" },
        { 53, "slash" },
        { 54, "right.shift" },
        { 55, "keypad.multiply" },
        { 56, "left.alt" },
        { 57, "space" },
        { 58, "caps.lock" },
        { 59, "f1" },
        { 60, "f2" },
        { 61, "f3" },
        { 62, "f4" },
        { 63, "f5" },
        { 64, "f6" },
        { 65, "f7" },
        { 66, "f8" },
        { 67, "f9" },
        { 68, "f10" },
        { 69, "num.lock" },
        { 70, "scroll.lock" },
        { 71, "keypad.7" },
        { 72, "keypad.8" },
        { 73, "keypad.9" },
        { 74, "keypad.subtract" },
        { 75, "keypad.4" },
        { 76, "keypad.5" },
        { 77, "keypad.6" },
        { 78, "keypad.add" },
        { 79, "keypad.1" },
        { 80, "keypad.2" },
        { 81, "keypad.3" },
        { 82, "keypad.0" },
        { 83, "keypad.period" },
        { 86, "world.1" },  // the extra key left of Z on ISO keyboards
        { 87, "f11" },
        { 88, "f12" },
        { 100, "f13" },
        { 101, "f14" },
        { 102, "f15" },
        { 103, "f16" },
        { 104, "f17" },
        { 105, "f18" },
        { 113, "f19" },
        { 141, "keypad.equal" },
        { 156, "keypad.enter" },
        { 157, "right.control" },
        { 181, "keypad.divide" },
        { 183, "print.screen" },
        { 184, "right.alt" },
        { 197, "pause" },
        { 199, "home" },
        { 200, "up" },
        { 201, "page.up" },
        { 203, "left" },
        { 205, "right" },
        { 207, "end" },
        { 208, "down" },
        { 209, "page.down" },
        { 210, "insert" },
        { 211, "delete" },
        { 219, "left.win" },
        { 220, "right.win" },
        { 221, "application" },
    };
    return names;
}

const QHash<QString, int>& set1Codes()
{
    static const QHash<QString, int> codes = [] {
        QHash<QString, int> result;
        for (auto it = set1Names().cbegin(); it != set1Names().cend(); ++it) {
            result.insert(it.value(), it.key());
        }
        return result;
    }();
    return codes;
}

// Linux evdev codes equal set 1 up to 88; the extended keys got their own numbers.
int evdevToSet1(int evdev)
{
    if (evdev >= 1 && evdev <= 88) {
        return evdev;
    }
    static const QHash<int, int> extended = {
        { 96, 156 },  { 97, 157 },  { 98, 181 },  { 99, 183 },  { 100, 184 }, { 102, 199 }, { 103, 200 }, { 104, 201 }, { 105, 203 },
        { 106, 205 }, { 107, 207 }, { 108, 208 }, { 109, 209 }, { 110, 210 }, { 111, 211 }, { 117, 141 }, { 119, 197 }, { 125, 219 },
        { 126, 220 }, { 127, 221 }, { 183, 100 }, { 184, 101 }, { 185, 102 }, { 186, 103 }, { 187, 104 }, { 188, 105 }, { 189, 113 },
    };
    return extended.value(evdev, 0);
}

int nativeSet1Code(const QKeyEvent* event)
{
    const auto scanCode = static_cast<int>(event->nativeScanCode());
    if (scanCode == 0) {
        return 0;
    }
    const auto platform = QGuiApplication::platformName();
    if (platform == "xcb" || platform == "wayland") {
        // XKB key codes are evdev codes offset by 8
        return evdevToSet1(scanCode - 8);
    }
    if (platform == "windows") {
        // bit 8 marks extended keys
        return (scanCode & 0x7f) | ((scanCode & 0x100) ? 0x80 : 0);
    }
    return 0;
}

// Fallback for platforms without usable scan codes. Only correct for keyboard layouts close to US QWERTY.
QString nameFromQtKey(const QKeyEvent* event)
{
    const int key = event->key();
    if (event->modifiers() & Qt::KeypadModifier) {
        if (key >= Qt::Key_0 && key <= Qt::Key_9) {
            return QString("keypad.%1").arg(key - Qt::Key_0);
        }
        switch (key) {
            case Qt::Key_Plus:
                return "keypad.add";
            case Qt::Key_Minus:
                return "keypad.subtract";
            case Qt::Key_Asterisk:
                return "keypad.multiply";
            case Qt::Key_Slash:
                return "keypad.divide";
            case Qt::Key_Enter:
                return "keypad.enter";
            case Qt::Key_Period:
            case Qt::Key_Comma:
                return "keypad.period";
            default:
                break;
        }
    }
    if (key >= Qt::Key_A && key <= Qt::Key_Z) {
        return QString(QChar('a' + (key - Qt::Key_A)));
    }
    if (key >= Qt::Key_0 && key <= Qt::Key_9) {
        return QString::number(key - Qt::Key_0);
    }
    if (key >= Qt::Key_F1 && key <= Qt::Key_F24) {
        return QString("f%1").arg(key - Qt::Key_F1 + 1);
    }
    static const QHash<int, QString> names = {
        { Qt::Key_Escape, "escape" },
        { Qt::Key_Tab, "tab" },
        { Qt::Key_Backspace, "backspace" },
        { Qt::Key_Return, "enter" },
        { Qt::Key_Enter, "keypad.enter" },
        { Qt::Key_Insert, "insert" },
        { Qt::Key_Delete, "delete" },
        { Qt::Key_Pause, "pause" },
        { Qt::Key_Print, "print.screen" },
        { Qt::Key_Home, "home" },
        { Qt::Key_End, "end" },
        { Qt::Key_Left, "left" },
        { Qt::Key_Up, "up" },
        { Qt::Key_Right, "right" },
        { Qt::Key_Down, "down" },
        { Qt::Key_PageUp, "page.up" },
        { Qt::Key_PageDown, "page.down" },
        { Qt::Key_Shift, "left.shift" },
        { Qt::Key_Control, "left.control" },
        { Qt::Key_Meta, "left.win" },
        { Qt::Key_Alt, "left.alt" },
        { Qt::Key_AltGr, "right.alt" },
        { Qt::Key_CapsLock, "caps.lock" },
        { Qt::Key_NumLock, "num.lock" },
        { Qt::Key_ScrollLock, "scroll.lock" },
        { Qt::Key_Menu, "application" },
        { Qt::Key_Space, "space" },
        { Qt::Key_Apostrophe, "apostrophe" },
        { Qt::Key_Comma, "comma" },
        { Qt::Key_Minus, "minus" },
        { Qt::Key_Period, "period" },
        { Qt::Key_Slash, "slash" },
        { Qt::Key_Semicolon, "semicolon" },
        { Qt::Key_Equal, "equal" },
        { Qt::Key_BracketLeft, "left.bracket" },
        { Qt::Key_Backslash, "backslash" },
        { Qt::Key_BracketRight, "right.bracket" },
        { Qt::Key_QuoteLeft, "grave.accent" },
    };
    return names.value(key);
}

// Key names of current (SDL) versions that GLFW versions spelled differently.
QString toGlfwName(const QString& name)
{
    static const QHash<QString, QString> renamed = {
        { "keypad.period", "keypad.decimal" },
        { "world.1", "world.2" },
        { "application", "menu" },
    };
    return renamed.value(name, name);
}

// LWJGL2 stored mouse buttons as negative numbers, starting at -100 for the left button.
int lwjgl2MouseCode(const QString& button)
{
    if (button == "left") {
        return -100;
    }
    if (button == "right") {
        return -99;
    }
    if (button == "middle") {
        return -98;
    }
    bool ok = false;
    const int number = button.toInt(&ok);
    return ok && number >= 4 ? -100 + number - 1 : 0;
}

std::optional<QString> keyForBackend(const QString& key, InputBackend backend)
{
    if (key.startsWith(mousePrefix)) {
        if (backend != InputBackend::Lwjgl2) {
            return key;
        }
        const int code = lwjgl2MouseCode(key.mid(mousePrefix.size()));
        return code != 0 ? std::optional(QString::number(code)) : std::nullopt;
    }
    if (!key.startsWith(keyboardPrefix)) {
        return std::nullopt;
    }
    const auto name = key.mid(keyboardPrefix.size());
    switch (backend) {
        case InputBackend::Sdl:
            return key;
        case InputBackend::Glfw:
            return keyboardPrefix + toGlfwName(name);
        case InputBackend::Lwjgl2:
            if (name == "unknown") {
                return QString("0");
            }
            if (const auto it = set1Codes().find(name); it != set1Codes().end()) {
                return QString::number(it.value());
            }
            return std::nullopt;
    }
    return std::nullopt;
}

QString actionForVersion(const QString& actionId, InputBackend backend, int dataVersion)
{
    if (actionId == "key.swapOffhand" && (backend == InputBackend::Lwjgl2 || dataVersion < swapOffhandDataVersion)) {
        return "key.swapHands";
    }
    return actionId;
}

}  // namespace

const QList<Action>& vanillaActions()
{
    static const QString movement = "key.categories.movement";
    static const QString gameplay = "key.categories.gameplay";
    static const QString inventory = "key.categories.inventory";
    static const QString creative = "key.categories.creative";
    static const QString multiplayer = "key.categories.multiplayer";
    static const QString misc = "key.categories.misc";
    static const QString spectator = "key.categories.spectator";

    // Defaults as of Minecraft 26.3
    static const QList<Action> actions = [] {
        QList<Action> list = {
            { "key.forward", movement, "key.keyboard.w" },
            { "key.left", movement, "key.keyboard.a" },
            { "key.back", movement, "key.keyboard.s" },
            { "key.right", movement, "key.keyboard.d" },
            { "key.jump", movement, "key.keyboard.space" },
            { "key.sneak", movement, "key.keyboard.left.shift" },
            { "key.sprint", movement, "key.keyboard.left.control" },
            { "key.attack", gameplay, "key.mouse.left" },
            { "key.use", gameplay, "key.mouse.right" },
            { "key.pickItem", gameplay, "key.mouse.middle" },
            { "key.inventory", inventory, "key.keyboard.e" },
            { "key.swapOffhand", inventory, "key.keyboard.f" },
            { "key.drop", inventory, "key.keyboard.q" },
        };
        for (int slot = 1; slot <= 9; ++slot) {
            list.append({ QString("key.hotbar.%1").arg(slot), inventory, QString("key.keyboard.%1").arg(slot) });
        }
        list.append({
            { "key.saveToolbarActivator", creative, "key.keyboard.c" },
            { "key.loadToolbarActivator", creative, "key.keyboard.x" },
            { "key.chat", multiplayer, "key.keyboard.t" },
            { "key.playerlist", multiplayer, "key.keyboard.tab" },
            { "key.command", multiplayer, "key.keyboard.slash" },
            { "key.friends", multiplayer, "key.keyboard.o" },
            { "key.socialInteractions", multiplayer, "key.keyboard.p" },
            { "key.screenshot", misc, "key.keyboard.f2" },
            { "key.togglePerspective", misc, "key.keyboard.f5" },
            { "key.smoothCamera", misc, "key.keyboard.unknown" },
            { "key.fullscreen", misc, "key.keyboard.f11" },
            { "key.advancements", misc, "key.keyboard.l" },
            { "key.quickActions", misc, "key.keyboard.g" },
            { "key.toggleGui", misc, "key.keyboard.f1" },
            { "key.toggleSpectatorShaderEffects", misc, "key.keyboard.f4" },
            { "key.spectatorOutlines", spectator, "key.keyboard.unknown" },
            { "key.spectatorHotbar", spectator, "key.mouse.middle" },
        });
        return list;
    }();
    return actions;
}

QString actionDisplayName(const QString& actionId)
{
    static const QHash<QString, const char*> names = {
        { "key.forward", QT_TRANSLATE_NOOP("KeybindProfile", "Walk Forwards") },
        { "key.left", QT_TRANSLATE_NOOP("KeybindProfile", "Strafe Left") },
        { "key.back", QT_TRANSLATE_NOOP("KeybindProfile", "Walk Backwards") },
        { "key.right", QT_TRANSLATE_NOOP("KeybindProfile", "Strafe Right") },
        { "key.jump", QT_TRANSLATE_NOOP("KeybindProfile", "Jump") },
        { "key.sneak", QT_TRANSLATE_NOOP("KeybindProfile", "Sneak") },
        { "key.sprint", QT_TRANSLATE_NOOP("KeybindProfile", "Sprint") },
        { "key.attack", QT_TRANSLATE_NOOP("KeybindProfile", "Attack/Destroy") },
        { "key.use", QT_TRANSLATE_NOOP("KeybindProfile", "Use Item/Place Block") },
        { "key.pickItem", QT_TRANSLATE_NOOP("KeybindProfile", "Pick Block") },
        { "key.inventory", QT_TRANSLATE_NOOP("KeybindProfile", "Open/Close Inventory") },
        { "key.swapOffhand", QT_TRANSLATE_NOOP("KeybindProfile", "Swap Item With Offhand") },
        { "key.drop", QT_TRANSLATE_NOOP("KeybindProfile", "Drop Selected Item") },
        { "key.saveToolbarActivator", QT_TRANSLATE_NOOP("KeybindProfile", "Save Hotbar Activator") },
        { "key.loadToolbarActivator", QT_TRANSLATE_NOOP("KeybindProfile", "Load Hotbar Activator") },
        { "key.chat", QT_TRANSLATE_NOOP("KeybindProfile", "Open Chat") },
        { "key.playerlist", QT_TRANSLATE_NOOP("KeybindProfile", "List Players") },
        { "key.command", QT_TRANSLATE_NOOP("KeybindProfile", "Open Command") },
        { "key.friends", QT_TRANSLATE_NOOP("KeybindProfile", "Friends") },
        { "key.socialInteractions", QT_TRANSLATE_NOOP("KeybindProfile", "Social Interactions Screen") },
        { "key.screenshot", QT_TRANSLATE_NOOP("KeybindProfile", "Take Screenshot") },
        { "key.togglePerspective", QT_TRANSLATE_NOOP("KeybindProfile", "Toggle Perspective") },
        { "key.smoothCamera", QT_TRANSLATE_NOOP("KeybindProfile", "Toggle Cinematic Camera") },
        { "key.fullscreen", QT_TRANSLATE_NOOP("KeybindProfile", "Toggle Fullscreen") },
        { "key.advancements", QT_TRANSLATE_NOOP("KeybindProfile", "Advancements") },
        { "key.quickActions", QT_TRANSLATE_NOOP("KeybindProfile", "Quick Actions") },
        { "key.toggleGui", QT_TRANSLATE_NOOP("KeybindProfile", "Hide GUI") },
        { "key.toggleSpectatorShaderEffects", QT_TRANSLATE_NOOP("KeybindProfile", "Toggle Spectator Shader Effects") },
        { "key.spectatorOutlines", QT_TRANSLATE_NOOP("KeybindProfile", "Highlight Players (Spectators)") },
        { "key.spectatorHotbar", QT_TRANSLATE_NOOP("KeybindProfile", "Spectator Actions") },
    };
    if (actionId.startsWith("key.hotbar.")) {
        return QCoreApplication::translate("KeybindProfile", "Hotbar Slot %1").arg(actionId.mid(QString("key.hotbar.").size()));
    }
    if (const auto it = names.find(actionId); it != names.end()) {
        return QCoreApplication::translate("KeybindProfile", it.value());
    }
    return actionId;
}

QString categoryDisplayName(const QString& category)
{
    static const QHash<QString, const char*> names = {
        { "key.categories.movement", QT_TRANSLATE_NOOP("KeybindProfile", "Movement") },
        { "key.categories.gameplay", QT_TRANSLATE_NOOP("KeybindProfile", "Gameplay") },
        { "key.categories.inventory", QT_TRANSLATE_NOOP("KeybindProfile", "Inventory") },
        { "key.categories.creative", QT_TRANSLATE_NOOP("KeybindProfile", "Creative Mode") },
        { "key.categories.multiplayer", QT_TRANSLATE_NOOP("KeybindProfile", "Multiplayer") },
        { "key.categories.misc", QT_TRANSLATE_NOOP("KeybindProfile", "Miscellaneous") },
        { "key.categories.spectator", QT_TRANSLATE_NOOP("KeybindProfile", "Spectator") },
    };
    if (const auto it = names.find(category); it != names.end()) {
        return QCoreApplication::translate("KeybindProfile", it.value());
    }
    return category;
}

QString keyDisplayName(const QString& key)
{
    if (key.isEmpty() || key == "key.keyboard.unknown") {
        return QCoreApplication::translate("KeybindProfile", "Not Bound");
    }
    if (key.startsWith(mousePrefix)) {
        const auto button = key.mid(mousePrefix.size());
        if (button == "left") {
            return QCoreApplication::translate("KeybindProfile", "Left Button");
        }
        if (button == "right") {
            return QCoreApplication::translate("KeybindProfile", "Right Button");
        }
        if (button == "middle") {
            return QCoreApplication::translate("KeybindProfile", "Middle Button");
        }
        return QCoreApplication::translate("KeybindProfile", "Mouse Button %1").arg(button);
    }
    if (key == "key.keyboard.world.1") {
        return QCoreApplication::translate("KeybindProfile", "ISO Extra Key (<>)");
    }
    // "key.keyboard.left.shift" -> "Left Shift"
    auto words = key.mid(keyboardPrefix.size()).split('.');
    for (auto& word : words) {
        if (!word.isEmpty()) {
            word[0] = word[0].toUpper();
        }
    }
    return words.join(' ');
}

Profile load()
{
    Profile profile;
    const auto json = QJsonDocument::fromJson(APPLICATION->settings()->get("KeybindProfile").toString().toUtf8()).object();
    for (auto it = json.begin(); it != json.end(); ++it) {
        if (it.value().isString()) {
            profile.insert(it.key(), it.value().toString());
        }
    }
    return profile;
}

void save(const Profile& profile)
{
    QJsonObject json;
    for (auto it = profile.cbegin(); it != profile.cend(); ++it) {
        json.insert(it.key(), it.value());
    }
    APPLICATION->settings()->set("KeybindProfile", QString::fromUtf8(QJsonDocument(json).toJson(QJsonDocument::Compact)));
}

QString keyFromEvent(const QKeyEvent* event)
{
    if (const int code = nativeSet1Code(event); code != 0) {
        if (const auto it = set1Names().find(code); it != set1Names().end()) {
            return keyboardPrefix + it.value();
        }
    }
    const auto name = nameFromQtKey(event);
    return name.isEmpty() ? QString() : keyboardPrefix + name;
}

QString keyFromMouseButton(Qt::MouseButton button)
{
    switch (button) {
        case Qt::LeftButton:
            return mousePrefix + "left";
        case Qt::RightButton:
            return mousePrefix + "right";
        case Qt::MiddleButton:
            return mousePrefix + "middle";
        case Qt::BackButton:
            return mousePrefix + "4";
        case Qt::ForwardButton:
            return mousePrefix + "5";
        case Qt::ExtraButton3:
            return mousePrefix + "6";
        case Qt::ExtraButton4:
            return mousePrefix + "7";
        case Qt::ExtraButton5:
            return mousePrefix + "8";
        default:
            return {};
    }
}

QString buildOptions(const Profile& profile, InputBackend backend, int dataVersion, QStringList* skipped)
{
    QStringList lines;
    if (backend != InputBackend::Lwjgl2) {
        lines << QString("version:%1").arg(dataVersion);
    }
    for (auto it = profile.cbegin(); it != profile.cend(); ++it) {
        const auto key = keyForBackend(it.value(), backend);
        if (!key) {
            if (skipped) {
                *skipped << it.key();
            }
            continue;
        }
        lines << QString("key_%1:%2").arg(actionForVersion(it.key(), backend, dataVersion), *key);
    }
    return lines.join('\n') + '\n';
}

}  // namespace KeybindProfile
