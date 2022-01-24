/*************************************************************************
 * This file is part of input-overlay
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

#pragma once
#include "item.hpp"
#include <obs.hpp>

class SourceItem : public LayoutItem {
    Q_OBJECT
    OBSSource m_src;
    OBSSignal removedSignal;
public:
    static void OBSSourceRemoved(void *data, calldata_t *params);
    SourceItem(Layout* parent, int x, int y, int w = 1, int h = 1);
    ~SourceItem();

    void SetSource(obs_source_t* src);
    virtual void Render(const Config &cfg) override;
};
