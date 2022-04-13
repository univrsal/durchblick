/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>
    Copyright (C) 2014 by Zachary Lund <admin@computerquip.com>

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

#pragma once
#include <QTimer>
#include <QWidget>
#if defined(_WIN32)
#    include <Windows.h>
#endif
namespace PlatformUtil {
inline bool IsAlwaysOnTop(QWidget* window)
{
#if defined(__linux__) || defined(__FreeBSD__) || defined(__DragonFly__)
    return (window->windowFlags() & Qt::WindowStaysOnTopHint) != 0;
#elif defined(_WIN32)
    DWORD exStyle = GetWindowLong((HWND)window->winId(), GWL_EXSTYLE);
    return (exStyle & WS_EX_TOPMOST) != 0;
#else
    return false;
#endif
}

inline void SetAlwaysOnTop(QWidget* window, bool enable, bool reshow = true)
{
#if defined(__linux__) || defined(__FreeBSD__) || defined(__DragonFly__)
    Qt::WindowFlags flags = window->windowFlags();

    if (enable)
        flags |= Qt::WindowStaysOnTopHint;
    else
        flags &= ~Qt::WindowStaysOnTopHint;

    window->setWindowFlags(flags);
    if (reshow) {
        window->hide();
        QTimer::singleShot(50, window, [window] { window->show(); });
    }
#elif defined(_WIN32)
    HWND hwnd = (HWND)window->winId();
    SetWindowPos(hwnd, enable ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
#endif
}

}
