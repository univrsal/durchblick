/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "windows_helper.hpp"
#include <vector>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct MonitorData {
    const wchar_t* id;
    MONITORINFOEX info;
    bool found;
};

static BOOL CALLBACK GetMonitorCallback(HMONITOR monitor, HDC, LPRECT,
    LPARAM param)
{
    MonitorData* data = (MonitorData*)param;

    if (GetMonitorInfoW(monitor, &data->info)) {
        if (wcscmp(data->info.szDevice, data->id) == 0) {
            data->found = true;
            return false;
        }
    }

    return true;
}

#define GENERIC_MONITOR_NAME QStringLiteral("Generic PnP Monitor")

QString GetMonitorName(const QString& id)
{
    MonitorData data = {};
    data.id = (const wchar_t*)id.utf16();
    data.info.cbSize = sizeof(data.info);

    EnumDisplayMonitors(nullptr, nullptr, GetMonitorCallback,
        (LPARAM)&data);
    if (!data.found) {
        return GENERIC_MONITOR_NAME;
    }

    UINT32 numPath, numMode;
    if (GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &numPath,
            &numMode)
        != ERROR_SUCCESS) {
        return GENERIC_MONITOR_NAME;
    }

    std::vector<DISPLAYCONFIG_PATH_INFO> paths(numPath);
    std::vector<DISPLAYCONFIG_MODE_INFO> modes(numMode);

    if (QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &numPath, paths.data(),
            &numMode, modes.data(),
            nullptr)
        != ERROR_SUCCESS) {
        return GENERIC_MONITOR_NAME;
    }

    DISPLAYCONFIG_TARGET_DEVICE_NAME target;
    bool found = false;

    paths.resize(numPath);
    for (size_t i = 0; i < numPath; ++i) {
        const DISPLAYCONFIG_PATH_INFO& path = paths[i];

        DISPLAYCONFIG_SOURCE_DEVICE_NAME s;
        s.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
        s.header.size = sizeof(s);
        s.header.adapterId = path.sourceInfo.adapterId;
        s.header.id = path.sourceInfo.id;

        if (DisplayConfigGetDeviceInfo(&s.header) == ERROR_SUCCESS && wcscmp(data.info.szDevice, s.viewGdiDeviceName) == 0) {
            target.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
            target.header.size = sizeof(target);
            target.header.adapterId = path.sourceInfo.adapterId;
            target.header.id = path.targetInfo.id;
            found = DisplayConfigGetDeviceInfo(&target.header) == ERROR_SUCCESS;
            break;
        }
    }

    if (!found) {
        return GENERIC_MONITOR_NAME;
    }

    return QString::fromWCharArray(target.monitorFriendlyDeviceName);
}
