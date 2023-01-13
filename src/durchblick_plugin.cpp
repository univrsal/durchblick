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
#include "items/registry.hpp"
#include "plugin-macros.generated.h"
#include "ui/durchblick.hpp"
#include "util/util.h"
#include <QAction>
#include <obs-frontend-api.h>
#include <obs-module.h>
#include <thread>
#include <util/util.hpp>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("durchblick", "en-US")

bool obs_module_load()
{
    binfo("Loading v%s build time %s", PLUGIN_VERSION, BUILD_TIME);

    // Speeds up loading
    std::thread reg([] { Registry::RegisterDefaults(); });
    reg.detach();

    Registry::RegisterCustomWidgetProcedure();

    QAction::connect(static_cast<QAction*>(obs_frontend_add_tools_menu_qaction(T_MENU_OPTION)),
        &QAction::triggered, [] {
            auto layouts = Config::LoadLayoutsForCurrentSceneCollection();
            Config::db->CreateDisplay(true);
            if (layouts.size() > 0)
                Config::db->Load(layouts[0].toObject());
            else
                Config::db->GetLayout()->CreateDefaultLayout();
            Config::db->show();
        });

    Config::RegisterCallbacks();
    Config::Load();
    return true;
}

void obs_module_post_load()
{
}

void obs_module_unload()
{
    Registry::Free();
}
