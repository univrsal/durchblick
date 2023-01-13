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
#include "../util/util.h"
#include "audio_mixer.hpp"
#include "custom_item.hpp"
#include "preview_program_item.hpp"
#include "scene_item.hpp"
#include "source_item.hpp"

namespace Registry {

std::mutex EntryMutex;
QList<ItemRegistry::Entry> ItemRegistry::Entries;
QList<std::function<void()>> ItemRegistry::DeinitCallbacks;

void ItemRegistry::Register(Constructor const& c, char const* n, char const* id)
{
    // Make sure internal types are always at the beginning (index 0 is the placeholder)
    if (Entries.empty())
        Entries.insert(0, Entry { c, id, n });
    else
        Entries.insert(1, Entry { c, id, n });
}

void ItemRegistry::RegisterCustom(DurchblickCallbacks const* Callbacks)
{
#define CHECK_CB(cb)                                                                                     \
    if (!cb) {                                                                                           \
        berr("Missing required callback " #cb " for custom widget '%s'. Aborting.", Callbacks->GetId()); \
        return;                                                                                          \
    }

    if (!Callbacks->GetId) {
        berr("Missing required callback GetId for custom widget. Aborting.");
        return;
    }

    if (!Callbacks->GetId() || strlen(Callbacks->GetId()) < 1) {
        berr("Custom widget returns invalid id. Aborting.");
        return;
    }

    CHECK_CB(Callbacks->Init);
    CHECK_CB(Callbacks->Destroy);
    CHECK_CB(Callbacks->GetName);
    CHECK_CB(Callbacks->Render);

    if (!Callbacks->GetName() || strlen(Callbacks->GetName()) < 1) {
        berr("Custom widget returns invalid display name. Aborting.");
        return;
    }

    auto* id = Callbacks->GetId();
    auto* n = Callbacks->GetName();

    EntryMutex.lock();
    Entries.append(Entry { id, n });
    auto* e = &Entries.last();
    e->cbs = *Callbacks;
    Entries.last().construct = [e](Layout* p, int x, int y, int w, int h) {
        return new CustomItem(p, e->cbs, x, y, w, h);
    };
    EntryMutex.unlock();
}

void Free()
{
    for (auto& Callback : ItemRegistry::DeinitCallbacks)
        Callback();
}

void RegisterDefaults()
{
    std::lock_guard<std::mutex> lock(EntryMutex);
    // Keep this one first otherwise it'll mess up the dialog combobox
    Registry::Register<PlaceholderItem>("PlaceholderItem");

    // Last one shows up first in the combobox
    Registry::Register<PreviewProgramItem>(T_WIDGET_PREVIEW_PROGRAM);
    Registry::Register<SourceItem>(T_WIDGET_SOURCE);
    Registry::Register<AudioMixerItem>(T_WIDGET_AUDIO_MIXER);
    Registry::Register<SceneItem>(T_WIDGET_SCENE);
    
    Registry::AddCallbacks<SourceItem>();
}

LayoutItem* MakeItem(Layout* l, QJsonObject const& obj)
{
    QString id = obj["id"].toString();

    if (id == "CustomItem")
        id = obj["custom_id"].toString();

    for (auto const& Entry : qAsConst(ItemRegistry::Entries)) {
        if (Entry.id == id) {
            auto* item = Entry.construct(l, 0, 0, 0, 0);
            item->ReadFromJson(obj);
            return item;
        }
    }

    return nullptr;
}

void RegisterCustomWidgetProcedure()
{
    auto* sh = obs_get_proc_handler();

    proc_handler_add(
        sh, "void durchblick_register_custom_widget(in ptr callbacks, in int api_version)", [](void*, calldata_t* data) {
            long long version = -1;
            if (calldata_get_int(data, "api_version", &version)) {
                if (version != DURCHBLICK_CUSTOM_WIDGET_API_VERSION) {
                    berr("Tried to register custom widget with api version %lli, but we're using version %i.", version, DURCHBLICK_CUSTOM_WIDGET_API_VERSION);
                    return;
                }
            } else {
                berr("Failed to get api_version from calldata. Can't register custom widget.");
                return;
            }

            DurchblickCallbacks* ptr {};
            if (calldata_get_ptr(data, "callbacks", &ptr) && ptr) {
                ItemRegistry::RegisterCustom(static_cast<DurchblickCallbacks*>(ptr));
            } else {
                berr("Failed to get callbacks from calldata. Can't register custom widget.");
            }
        },
        nullptr);
}

const ItemRegistry::Entry* GetEntryById(const QString& id)
{
    for (auto const& Entry : qAsConst(ItemRegistry::Entries)) {
        if (Entry.id == id)
            return &Entry;
    }
    return nullptr;
}

}
