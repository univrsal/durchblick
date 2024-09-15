/*************************************************************************
 * This file is part of durchblick
 * git.vrsal.xyz/alex/durchblick
 * Copyright 2023 univrsal <uni@vrsal.xyz>.
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
#include "../util/callbacks.h"
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
    using Constructor = std::function<LayoutItem*(Layout*, int, int, int, int)>;

    struct Entry {
        Constructor construct {};
        QString name {}, id {};
        DurchblickCallbacks cbs {};

        Entry(Constructor const& c, const char* _id, const char* _name)
            : construct(c)
            , name(utf8_to_qt(_name))
            , id(utf8_to_qt(_id))
            , cbs({})
        {
        }

        Entry(const char* _id, const char* _name)
            : name(utf8_to_qt(_name))
            , id(utf8_to_qt(_id))
            , cbs({})
        {
        }
    };

    static QList<Entry> Entries;

    static QList<std::function<void()>> DeinitCallbacks;

    static void Register(Constructor const&, char const*, char const*);
    static void RegisterCustom(DurchblickCallbacks const* Callbacks);
};

extern void RegisterDefaults();
extern LayoutItem* MakeItem(Layout* l, QJsonObject const& obj);
extern ItemRegistry::Entry const* GetEntryById(QString const&);
extern void Free();
extern void RegisterCustomWidgetProcedure();

template<class T>
void Register(char const* name)
{
    ItemRegistry::Register([](Layout* p, int x, int y, int w, int h) {
        return new T(p, x, y, w, h);
    },
        name, T::staticMetaObject.className());
}

template<class T>
void AddCallbacks()
{
    T::Init();
    ItemRegistry::DeinitCallbacks.append(T::Deinit);
}

}
