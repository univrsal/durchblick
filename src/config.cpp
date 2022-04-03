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
#include "durchblick.hpp"
#include "util.h"
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

void Load()
{
    BPtr<char> path = obs_module_config_path("layout.json");
    BPtr<char> sc = obs_frontend_get_current_scene_collection();

    QFile f(utf8_to_qt(path));
    QDir dir(utf8_to_qt(path));

    db = new Durchblick;

    // Make sure that the plugin config folder exists
    if (!dir.cd("../..")) { // cd into plugin_config
        berr("Failed to change directory from '%s'. Cannot save/load layouts.", path.Get());
        return;
    }

    if (!dir.cd("durchblick") && !dir.mkdir("durchblick")) {
        berr("Failed to create config folder '%s'. Cannot save/load layouts.", path.Get());
        return;
    }

    if (f.exists()) {
        if (f.open(QIODevice::ReadOnly)) {
            QJsonDocument doc;
            doc = QJsonDocument::fromJson(f.readAll());
            f.close();
            Cfg = doc.object();
            auto layouts = Cfg[utf8_to_qt(sc)].toArray();
            db->Load(layouts[0].toObject());
        }
    }

    if (db->GetLayout()->IsEmpty())
        db->GetLayout()->CreateDefaultLayout();
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
