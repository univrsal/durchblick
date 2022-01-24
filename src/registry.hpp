/*************************************************************************
 * This file is part of input-overlay
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
#include <functional>
#include <QList>
#include <QString>

class LayoutItem;
class Layout;

namespace Registry {
class ItemRegistry {
public:
    using Constructor = std::function<LayoutItem*(Layout*, int, int, int, int, void*)>;
    struct Entry {
        Constructor construct;
        void* priv{};
        QString name;
    };
    static QList<Entry> Entries;

    static void Register(Constructor const&, char const*, void* = nullptr);
};

extern void RegisterDefaults();

template<class T>
void Register(char const* name, void* = nullptr)
{
    ItemRegistry::Register([](Layout* p, int x, int y, int w, int h, void*) {
        return new T(p, x, y, w, h);
    }, name, nullptr);
}
}
