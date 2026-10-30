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

#include <QWidget>

#include "minecraft/KeybindProfile.h"
#include "ui/pages/BasePage.h"

class QLabel;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;

class KeybindsPage : public QWidget, public BasePage {
    Q_OBJECT

   public:
    explicit KeybindsPage(QWidget* parent = nullptr);
    ~KeybindsPage() override = default;

    QString displayName() const override { return tr("Keybinds"); }
    QIcon icon() const override { return QIcon::fromTheme("custom-commands"); }
    QString id() const override { return "keybind-settings"; }
    bool apply() override;
    void retranslate() override;

   private:
    void buildTree();
    void updateItem(QTreeWidgetItem* item);
    void changeSelected();
    void resetSelected();
    void resetAll();
    void updateButtons();

    KeybindProfile::Profile m_profile;
    QLabel* m_description;
    QTreeWidget* m_tree;
    QPushButton* m_changeButton;
    QPushButton* m_resetButton;
    QPushButton* m_resetAllButton;
};
