/*************************************************************************
 * This file is part of durchblick
 * git.vrsal.xyz/alex/durchblick
 * Copyright 2022 univrsal <uni@vrsal.xyz>.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *************************************************************************/

#pragma once
#include "item.hpp"
#include <QJsonObject>
#include <QList>
#include <QString>
#include <functional>

class LayoutItem;
class Layout;

namespace Registry {
class ItemRegistry {
public:
    using Constructor = std::function<LayoutItem*(Layout*, int, int, int, int, void*)>;
    struct Entry {
        Constructor construct;
        void* priv {};
        QString name, id;
    };
    static QList<Entry> Entries;
    static QList<std::function<void()>> DeinitCallbacks;

    static void Register(Constructor const&, char const*, char const*, void* = nullptr);
};

extern void RegisterDefaults();
extern LayoutItem* MakeItem(Layout* l, QJsonObject const& obj);
extern void Free();

template<class T>
void Register(char const* name)
{
    ItemRegistry::Register([](Layout* p, int x, int y, int w, int h, void*) {
        return new T(p, x, y, w, h);
    },
        name, T::staticMetaObject.className(), nullptr);
}

template<class T>
void AddCallbacks()
{
    T::Init();
    ItemRegistry::DeinitCallbacks.append(T::Deinit);
}

}
