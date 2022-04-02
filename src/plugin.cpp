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
#include "registry.hpp"
#include "source_item.hpp"
#include "util.h"
#include "util/util.hpp"
#include <QAction>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLayout>
#include <QMainWindow>
#include <QVBoxLayout>
#include <obs-module.h>
#include <thread>
#if _WIN32
#    include <obs-frontend-api.h>
#else
#    include <obs/obs-frontend-api.h>
#endif

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("durchblick", "en-US")

bool obs_module_load()
{
    binfo("Loading v%s build time %s", PLUGIN_VERSION, BUILD_TIME);

    // Speeds up loading
    std::thread reg([] { Registry::RegisterDefaults(); });
    reg.detach();

    QAction::connect(static_cast<QAction*>(obs_frontend_add_tools_menu_qaction(T_MENU_OPTION)),
        &QAction::triggered, [] {
            Config::db->show();
        });

    auto FinishedLoadingCallback = [](enum obs_frontend_event event, void*) {
        if (event == OBS_FRONTEND_EVENT_FINISHED_LOADING)
            Config::Load();
    };

    auto ExitCallback = [](enum obs_frontend_event event, void*) {
        // I couldn't find another event that was on exit and
        // before source/scene data was cleared
        if (event == OBS_FRONTEND_EVENT_SCRIPTING_SHUTDOWN) {
            // Final save and cleanup
            Config::Save();
            Config::Cleanup();
        }
    };

    obs_frontend_add_event_callback(ExitCallback, nullptr);
    obs_frontend_add_event_callback(FinishedLoadingCallback, nullptr);

    return true;
}

void obs_module_unload()
{
    Registry::Free();
}
