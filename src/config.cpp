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
#include "ui/durchblick_dock.hpp"
#include "util/util.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <obs-frontend-api.h>
#include <obs-module.h>
#include <util/platform.h>
#include <util/util.hpp>

#if !defined(_WIN32) && !defined(__APPLE__)
#    include <obs-nix-platform.h>
#endif
namespace Config {

Durchblick* db = nullptr;
DurchblickDock* dbdock = nullptr;

QJsonObject Cfg;

QJsonArray LoadLayoutsForCurrentSceneCollection()
{
    BPtr<char> path = obs_module_config_path("layout.json");
    BPtr<char> sc = obs_frontend_get_current_scene_collection();
    BPtr<char> folder = obs_module_config_path("");

    if (os_mkdirs(folder) == MKDIR_ERROR) {
        berr("Failed to change directory from '%s'. Cannot save/load layouts.", folder.Get());
        return {};
    }

    QFile f(utf8_to_qt(path.Get()));

    if (f.exists()) {
        if (f.open(QIODevice::ReadOnly)) {
            QJsonDocument doc;
            doc = QJsonDocument::fromJson(f.readAll());
            f.close();
            Cfg = doc.object();
            auto layouts = Cfg[utf8_to_qt(sc.Get())].toArray();
            if (layouts.isEmpty()) {
                berr("No layouts found");
                return {};
            }
            return layouts;
        }
    }
    return {};
}

void RegisterCallbacks()
{
    obs_frontend_add_save_callback([](obs_data_t*, bool, void*) {
        // Refresh this flag because if the user changed the "Hide OBS window from display capture setting"
        // durchblick would otherwise suddenly show up again
        if (db)
            db->SetHideFromDisplayCapture(db->GetHideFromDisplayCapture());
    },
        nullptr);

    obs_frontend_add_event_callback([](enum obs_frontend_event event, void*) {
        if (event == OBS_FRONTEND_EVENT_FINISHED_LOADING) {
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
            auto layouts = LoadLayoutsForCurrentSceneCollection();
            db->Load(layouts[0].toObject());
        }
    },
        nullptr);
}

void Load()
{
    auto layouts = LoadLayoutsForCurrentSceneCollection();

    if (!db)
        db = new Durchblick;

    if (layouts.size() > 0)
        db->Load(layouts[0].toObject());
    else
        db->setVisible(false);

#if !defined(_WIN32) && !defined(__APPLE__)
    if (obs_get_nix_platform() > OBS_NIX_PLATFORM_X11_EGL)
#endif
    {
        if (!dbdock) {
            const auto main_window = static_cast<QMainWindow*>(obs_frontend_get_main_window());
            obs_frontend_push_ui_translation(obs_module_get_string);
            dbdock = new DurchblickDock((QWidget*)main_window);
            obs_frontend_add_dock(Config::dbdock);
            obs_frontend_pop_ui_translation();
        }

        if (layouts.size() > 1)
            dbdock->GetDurchblick()->Load(layouts[1].toObject());
        else
            dbdock->setVisible(false);
    }
}

void Save()
{

    QJsonArray layouts {};
    QJsonObject obj1 {}, obj2 {};
    BPtr<char> path = obs_module_config_path("layout.json");
    BPtr<char> sc = obs_frontend_get_current_scene_collection();
    QFile f(utf8_to_qt(path.Get()));

    if (db) {
        db->Save(obj1);
        layouts.append(obj1);
    } else {
        layouts.append({});
    }

    if (dbdock) {
        dbdock->GetDurchblick()->Save(obj2);
        layouts.append(obj2);
    } else {
        layouts.append({});
    }

    Cfg[utf8_to_qt(sc.Get())] = layouts;
    if (f.open(QIODevice::WriteOnly)) {
        QJsonDocument doc;
        doc.setObject(Cfg);
        auto data = doc.toJson();
        auto wrote = f.write(data);

        if (data.length() != wrote) {
            berr("Couldn't write config file to %s, only"
                 "wrote %lli bytes out of %i",
                path.Get(), wrote, int(data.length()));
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
    dbdock->deleteLater();
    dbdock = nullptr;
}

}
