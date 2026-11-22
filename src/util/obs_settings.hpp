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

#include <obs-frontend-api.h>
#include <obs-module.h>
#include <util/config-file.h>

// OBS Studio 31 moved most "BasicWindow" settings (multiview mouse switching,
// transition on double click, projector options, ...) from the global app
// config (global.ini) to the per-user config (user.ini). Reading them from
// obs_frontend_get_app_config() on OBS 31+ always returns the default (false),
// which broke clicking scenes in the multiview.
//
// This helper reads the user config first and falls back to the app config so
// it keeps working on both old and new OBS versions.
namespace ObsSettings {

inline config_t* UserConfig()
{
#if LIBOBS_API_MAJOR_VER >= 31
    return obs_frontend_get_user_config();
#else
    return obs_frontend_get_global_config();
#endif
}

inline config_t* AppConfig()
{
#if LIBOBS_API_MAJOR_VER >= 31
    return obs_frontend_get_app_config();
#else
    return obs_frontend_get_global_config();
#endif
}

inline bool GetBasicWindowBool(const char* name)
{
    config_t* user = UserConfig();
    if (user && config_has_user_value(user, "BasicWindow", name))
        return config_get_bool(user, "BasicWindow", name);

    config_t* app = AppConfig();
    if (app && config_has_user_value(app, "BasicWindow", name))
        return config_get_bool(app, "BasicWindow", name);

    // Nothing explicitly set, use the default from whichever config has one
    if (user)
        return config_get_bool(user, "BasicWindow", name);
    return app ? config_get_bool(app, "BasicWindow", name) : false;
}

}
