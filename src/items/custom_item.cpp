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

#include "custom_item.hpp"
#include <QJsonArray>
#include <QJsonDocument>

CustomItem::CustomItem(Layout* parent, DurchblickCallbacks const& cb, int x, int y, int w, int h)
    : LayoutItem(parent, x, y, w, h)
    , m_cb_data(cb)
{
    PrivateData = m_cb_data.Init(this);
}

CustomItem::~CustomItem()
{
    m_cb_data.Destroy(this, PrivateData);
}

void CustomItem::Update(DurchblickItemConfig const& cfg)
{
    LayoutItem::Update(cfg);
    if (m_cb_data.Update)
        m_cb_data.Update(this, PrivateData, &cfg, m_cell.col, m_cell.row, m_cell.w, m_cell.h);
}

void CustomItem::ContextMenu(QMenu& m)
{
    LayoutItem::ContextMenu(m);
    if (m_cb_data.ContextMenu)
        m_cb_data.ContextMenu(this, PrivateData, &m);
}

void CustomItem::MouseEvent(MouseData const& e, DurchblickItemConfig const& cfg)
{
    LayoutItem::MouseEvent(e, cfg);
    if (m_cb_data.MouseEvent)
        m_cb_data.MouseEvent(this, PrivateData, &cfg, e.x, e.y, e.buttons, e.modifiers);
}

void CustomItem::Render(DurchblickItemConfig const& cfg)
{
    LayoutItem::Render(cfg);
    m_cb_data.Render(this, PrivateData, &cfg);
}

void CustomItem::WriteToJson(QJsonObject& Obj)
{
    LayoutItem::WriteToJson(Obj);
    Obj["custom_id"] = m_cb_data.GetId();

    if (m_cb_data.Save) {
        auto* json = m_cb_data.Save(this, PrivateData);
        // I assume iterating the objects would be faster than encoding and then decoding again
        // but I'm lazy.
        auto* c = json_dumps(json, JSON_COMPACT);
        QJsonParseError err;
        QJsonDocument tmp = QJsonDocument::fromJson(c, &err);

        if (err.error == QJsonParseError::NoError && !tmp.isNull()) {
            if (tmp.isObject())
                Obj["custom_data"] = tmp.object();
            else
                Obj["custom_data"] = tmp.array();
        } else {
            berr("Failed to write custom widget data for '%s' error: %s",
                m_cb_data.GetId(), qt_to_utf8(err.errorString()));
        }

        free(c);
        json_decref(json);
    }
}

void CustomItem::ReadFromJson(const QJsonObject& Obj)
{
    LayoutItem::ReadFromJson(Obj);
    if (m_cb_data.Load && Obj.contains("custom_data")) {
        auto data = Obj["custom_data"];
        QJsonDocument tmp;
        if (data.isObject())
            tmp.setObject(data.toObject());
        else if (data.isArray())
            tmp.setArray(data.toArray());

        if (!tmp.isNull()) {
            auto string = tmp.toJson(QJsonDocument::Compact);
            json_error_t err;
            auto* json = json_loads(string, 0, &err);

            if (json) {
                m_cb_data.Load(this, PrivateData, json);
                json_decref(json);
            } else {
                berr("Failed to load custom widget data for '%s' error (line %i, col %i): %s",
                    m_cb_data.GetId(), err.line, err.column, err.text);
            }
        }
    }
}

uint32_t CustomItem::GetFillColor()
{
    if (m_cb_data.GetFillColor)
        return m_cb_data.GetFillColor(this, PrivateData);
    return LayoutItem::GetFillColor();
}
