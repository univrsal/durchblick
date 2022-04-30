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

#include "config.hpp"
#include "ui/durchblick.hpp"
#include "util/util.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <obs-module.h>
#include <util/util.hpp>
#if _WIN32
#    include <obs-frontend-api.h>
#else
#    include <obs/obs-frontend-api.h>
#endif

namespace Config {

Durchblick* db = nullptr;
QJsonObject Cfg;
QJsonObject LoadLayoutForCurrentSceneCollection()
{
    BPtr<char> path = obs_module_config_path("layout.json");
    BPtr<char> sc = obs_frontend_get_current_scene_collection();
    QFile f(utf8_to_qt(path));
    QDir dir(utf8_to_qt(path));

    // Make sure that the plugin config folder exists
    if (!dir.cd("../..")) { // cd into plugin_config
        berr("Failed to change directory from '%s'. Cannot save/load layouts.", path.Get());
        return {};
    }

    if (!dir.cd("durchblick") && !dir.mkdir("durchblick")) {
        berr("Failed to create config folder '%s'. Cannot save/load layouts.", path.Get());
        return {};
    }

    if (f.exists()) {
        if (f.open(QIODevice::ReadOnly)) {
            QJsonDocument doc;
            doc = QJsonDocument::fromJson(f.readAll());
            f.close();
            Cfg = doc.object();
            auto layouts = Cfg[utf8_to_qt(sc)].toArray();
            if (layouts.isEmpty()) {
                berr("No layouts found");
                return {};
            }
            return layouts[0].toObject();
        }
    }
    return {};
}

void RegisterCallbacks()
{
    obs_frontend_add_save_callback([](obs_data_t* save_data, bool saving, void* private_data) {
        // Refresh this flag because if the user changed the "Hide OBS window from display capture setting"
        // durchblick would otherwise suddenly show up again
        if (db)
            db->SetHideFromDisplayCapture(db->GetHideFromDisplayCapture());
    },
        nullptr);

    obs_frontend_add_event_callback([](enum obs_frontend_event event, void*) {
        if (event == OBS_FRONTEND_EVENT_FINISHED_LOADING) {
            // Final save and cleanup
            Load();
        } else if (event == OBS_FRONTEND_EVENT_SCRIPTING_SHUTDOWN) {
            // I couldn't find another event that was on exit and
            // before source/scene data was cleared

            Save();
            Cleanup();
        } else if (event == OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGING) {
            Save(); // Save current layout
            db->GetLayout()->Clear();
        } else if (event == OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED) {
            db->Load(LoadLayoutForCurrentSceneCollection());
        }
    },
        nullptr);
}

void Load()
{
    db = new Durchblick;
    db->Load(LoadLayoutForCurrentSceneCollection());
}

void Save()
{
    if (!db)
        return;
    QJsonArray layouts;
    QJsonObject obj;
    BPtr<char> path = obs_module_config_path("layout.json");
    BPtr<char> sc = obs_frontend_get_current_scene_collection();
    QFile f(utf8_to_qt(path));

    db->Save(obj);
    layouts.append(obj);
    Cfg[utf8_to_qt(sc)] = layouts;
    if (f.open(QIODevice::WriteOnly)) {
        QJsonDocument doc;
        doc.setObject(Cfg);
        auto data = doc.toJson();
        auto wrote = f.write(data);

        if (data.length() != wrote) {
            berr("Couldn't write config file to %s, only"
                 "wrote %lli bytes out of %i",
                path.Get(), wrote, data.length());
        }
        f.close();
    } else {
        berr("Couldn't write config to %s", path.Get());
    }
}

void Cleanup()
{
    db->deleteLater();
    db = nullptr;
}

}
