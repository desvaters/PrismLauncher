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

#include "KeybindsPage.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <algorithm>
#include <functional>
#include <optional>

namespace {

constexpr int ActionIdRole = Qt::UserRole;

// Receives mouse buttons, so that they can be bound as well.
class MouseCaptureArea : public QLabel {
   public:
    MouseCaptureArea(std::function<void(Qt::MouseButton)> onPress, QWidget* parent) : QLabel(parent), m_onPress(std::move(onPress))
    {
        setFrameShape(QFrame::StyledPanel);
        setAlignment(Qt::AlignCenter);
        setMinimumSize(320, 100);
        setWordWrap(true);
    }

   protected:
    void mousePressEvent(QMouseEvent* event) override { m_onPress(event->button()); }

   private:
    std::function<void(Qt::MouseButton)> m_onPress;
};

/**
 * Waits for a key or mouse button. Every key is captured, including Escape, Tab and Enter, so the dialog can only be
 * left through its buttons, which therefore never take the focus.
 */
class KeyCaptureDialog : public QDialog {
   public:
    KeyCaptureDialog(const QString& actionName, const QString& defaultKey, QWidget* parent) : QDialog(parent)
    {
        setWindowTitle(KeybindsPage::tr("Change Keybind"));
        auto* layout = new QVBoxLayout(this);
        m_area = new MouseCaptureArea([this](Qt::MouseButton button) { acceptKey(KeybindProfile::keyFromMouseButton(button)); }, this);
        m_area->setText(KeybindsPage::tr("Press a key or click here with a mouse button to bind it to \"%1\".").arg(actionName));
        layout->addWidget(m_area);

        auto* buttons = new QDialogButtonBox(this);
        auto* unbind = buttons->addButton(KeybindsPage::tr("Unbind"), QDialogButtonBox::ActionRole);
        auto* reset = buttons->addButton(KeybindsPage::tr("Reset to Default (%1)").arg(KeybindProfile::keyDisplayName(defaultKey)),
                                         QDialogButtonBox::ResetRole);
        auto* cancel = buttons->addButton(QDialogButtonBox::Cancel);
        for (auto* button : { unbind, reset, cancel }) {
            button->setFocusPolicy(Qt::NoFocus);
        }
        connect(unbind, &QPushButton::clicked, this, [this] { acceptKey("key.keyboard.unknown"); });
        connect(reset, &QPushButton::clicked, this, [this] {
            m_resetToDefault = true;
            QDialog::accept();
        });
        connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
        layout->addWidget(buttons);
        setFocusPolicy(Qt::StrongFocus);
    }

    /// The chosen key, or nothing if the binding should be reset to the default.
    std::optional<QString> key() const { return m_resetToDefault ? std::nullopt : std::optional(m_key); }

   protected:
    bool event(QEvent* event) override
    {
        // intercepted here, because QDialog would use Escape, Enter and Tab otherwise
        if (event->type() == QEvent::KeyPress) {
            auto* keyEvent = static_cast<QKeyEvent*>(event);
            if (!keyEvent->isAutoRepeat()) {
                const auto key = KeybindProfile::keyFromEvent(keyEvent);
                if (key.isEmpty()) {
                    m_area->setText(KeybindsPage::tr("This key can't be used in Minecraft. Press another one."));
                } else {
                    acceptKey(key);
                }
            }
            return true;
        }
        return QDialog::event(event);
    }

   private:
    void acceptKey(const QString& key)
    {
        if (key.isEmpty()) {
            return;
        }
        m_key = key;
        QDialog::accept();
    }

    MouseCaptureArea* m_area;
    QString m_key;
    bool m_resetToDefault = false;
};

}  // namespace

KeybindsPage::KeybindsPage(QWidget* parent) : QWidget(parent), m_profile(KeybindProfile::load())
{
    auto* layout = new QVBoxLayout(this);
    m_description = new QLabel(this);
    m_description->setWordWrap(true);
    layout->addWidget(m_description);

    m_tree = new QTreeWidget(this);
    m_tree->setColumnCount(2);
    m_tree->setRootIsDecorated(false);
    m_tree->setUniformRowHeights(true);
    m_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    layout->addWidget(m_tree);

    auto* buttonLayout = new QHBoxLayout();
    m_changeButton = new QPushButton(this);
    m_resetButton = new QPushButton(this);
    m_resetAllButton = new QPushButton(this);
    buttonLayout->addWidget(m_changeButton);
    buttonLayout->addWidget(m_resetButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_resetAllButton);
    layout->addLayout(buttonLayout);

    connect(m_tree, &QTreeWidget::itemActivated, this, [this](QTreeWidgetItem* item) {
        if (!item->data(0, ActionIdRole).isNull()) {
            changeSelected();
        }
    });
    connect(m_tree, &QTreeWidget::itemSelectionChanged, this, &KeybindsPage::updateButtons);
    connect(m_changeButton, &QPushButton::clicked, this, &KeybindsPage::changeSelected);
    connect(m_resetButton, &QPushButton::clicked, this, &KeybindsPage::resetSelected);
    connect(m_resetAllButton, &QPushButton::clicked, this, &KeybindsPage::resetAll);

    retranslate();
}

void KeybindsPage::retranslate()
{
    m_description->setText(
        tr("These keybinds are written into the options.txt of newly created instances when they are launched for the first time. "
           "They are converted to the format of the instance's Minecraft version. Keybinds that a version doesn't have are ignored "
           "by it. Bold keybinds differ from the default."));
    m_tree->setHeaderLabels({ tr("Action"), tr("Key") });
    m_changeButton->setText(tr("&Change..."));
    m_resetButton->setText(tr("&Reset"));
    m_resetAllButton->setText(tr("Reset &All"));
    buildTree();
}

void KeybindsPage::buildTree()
{
    m_tree->clear();
    QTreeWidgetItem* categoryItem = nullptr;
    QString category;
    for (const auto& action : KeybindProfile::vanillaActions()) {
        if (action.category != category || !categoryItem) {
            category = action.category;
            categoryItem = new QTreeWidgetItem(m_tree, { KeybindProfile::categoryDisplayName(category) });
            categoryItem->setFlags(Qt::ItemIsEnabled);
            categoryItem->setFirstColumnSpanned(true);
            auto font = categoryItem->font(0);
            font.setBold(true);
            categoryItem->setFont(0, font);
        }
        auto* item = new QTreeWidgetItem(categoryItem, { KeybindProfile::actionDisplayName(action.id) });
        item->setData(0, ActionIdRole, action.id);
        updateItem(item);
    }
    m_tree->expandAll();
    updateButtons();
}

void KeybindsPage::updateItem(QTreeWidgetItem* item)
{
    const auto actionId = item->data(0, ActionIdRole).toString();
    const auto& actions = KeybindProfile::vanillaActions();
    const auto action = std::find_if(actions.begin(), actions.end(), [&](const auto& a) { return a.id == actionId; });
    const bool changed = m_profile.contains(actionId);
    item->setText(1, KeybindProfile::keyDisplayName(changed ? m_profile.value(actionId) : action->defaultKey));
    auto font = item->font(1);
    font.setBold(changed);
    item->setFont(0, font);
    item->setFont(1, font);
}

void KeybindsPage::changeSelected()
{
    auto* item = m_tree->currentItem();
    if (!item || item->data(0, ActionIdRole).isNull()) {
        return;
    }
    const auto actionId = item->data(0, ActionIdRole).toString();
    const auto& actions = KeybindProfile::vanillaActions();
    const auto action = std::find_if(actions.begin(), actions.end(), [&](const auto& a) { return a.id == actionId; });

    KeyCaptureDialog dialog(item->text(0), action->defaultKey, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    // a binding equal to the default is kept: older versions may have had a different default
    if (const auto key = dialog.key()) {
        m_profile.insert(actionId, *key);
    } else {
        m_profile.remove(actionId);
    }
    updateItem(item);
    updateButtons();
}

void KeybindsPage::resetSelected()
{
    auto* item = m_tree->currentItem();
    if (!item || item->data(0, ActionIdRole).isNull()) {
        return;
    }
    m_profile.remove(item->data(0, ActionIdRole).toString());
    updateItem(item);
    updateButtons();
}

void KeybindsPage::resetAll()
{
    m_profile.clear();
    buildTree();
}

void KeybindsPage::updateButtons()
{
    auto* item = m_tree->currentItem();
    const bool isAction = item && item->isSelected() && !item->data(0, ActionIdRole).isNull();
    m_changeButton->setEnabled(isAction);
    m_resetButton->setEnabled(isAction && m_profile.contains(item->data(0, ActionIdRole).toString()));
    m_resetAllButton->setEnabled(!m_profile.isEmpty());
}

bool KeybindsPage::apply()
{
    KeybindProfile::save(m_profile);
    return true;
}
