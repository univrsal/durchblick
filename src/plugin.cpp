/*************************************************************************
 * This file is part of input-overlay
 * git.vrsal.xyz/alex/durchblick
 * Copyright 2021 univrsal <uni@vrsal.xyz>.
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

#include <QAction>
#include <QMainWindow>
#include <obs-frontend-api.h>
#include <obs-module.h>
#include <QDialog>
#include <QLayout>
#include <QVBoxLayout>
#include "util.h"
#include "durchblick.hpp"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("durchblick", "en-US")

Test* dp = nullptr;


bool obs_module_load()
{
    binfo("Loading v%s build time %s", PLUGIN_VERSION, BUILD_TIME);

    /* UI registration from
     * https://github.com/Palakis/obs-websocket/
     */
    const auto menu_action = static_cast<QAction *>(obs_frontend_add_tools_menu_qaction(T_MENU_OPTION));
    obs_frontend_push_ui_translation(obs_module_get_string);
    const auto main_window = static_cast<QMainWindow *>(obs_frontend_get_main_window());
    obs_frontend_pop_ui_translation();

    const auto menu_cb = [main_window] {
        dp = new Test();
        dp->show();
    };
    QAction::connect(menu_action, &QAction::triggered, menu_cb);
    return true;
}

void obs_module_unload()
{
}
