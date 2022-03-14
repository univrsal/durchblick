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

#include "registry.hpp"
#include "preview_program_item.hpp"
#include "scene_item.hpp"
#include "source_item.hpp"
#include "util.h"

namespace Registry {

QList<ItemRegistry::Entry> ItemRegistry::Entries;
QList<std::function<void()>> ItemRegistry::DeinitCallbacks;

void ItemRegistry::Register(const Constructor& c, const char* n, const char* id, void* p)
{
    Entries.append(Entry { c, p, utf8_to_qt(n), id });
}

void Free()
{
    for (auto& Callback : ItemRegistry::DeinitCallbacks)
        Callback();
}

void RegisterDefaults()
{
    Registry::Register<SourceItem>(T_WIDGET_SOURCE);
    Registry::Register<SceneItem>(T_WIDGET_SCENE);
    Registry::Register<PreviewProgramItem>(T_WIDGET_PREVIEW_PROGRAM);

    // Keep this one last otherwise it'll mess up the dialog combobox
    Registry::Register<PlaceholderItem>("Placeholder");
    Registry::AddCallbacks<SourceItem>();
}

LayoutItem* MakeItem(Layout* l, const QJsonObject& obj)
{
    QString id = obj["id"].toString();
    for (auto const& Entry : qAsConst(ItemRegistry::Entries)) {
        if (Entry.id == id) {
            auto* item = Entry.construct(l, 0, 0, 0, 0, nullptr);
            item->ReadFromJson(obj);
            return item;
        }
    }
    return nullptr;
}

}
