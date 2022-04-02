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

#include "custom_item.hpp"
#include <QJsonArray>
#include <QJsonDocument>

CustomItem::CustomItem(Layout* parent, CallbackData const& cb, int x, int y, int w, int h)
    : LayoutItem(parent, x, y, w, h)
    , m_cb_data(cb)
{
    m_cb_data.PrivateData = m_cb_data.Init(this, x, y, w, h);
}

CustomItem::~CustomItem()
{
    m_cb_data.Destroy(this, m_cb_data.PrivateData);
}

void CustomItem::Update(DurchblickItemConfig const& cfg)
{
    if (m_cb_data.Update)
        m_cb_data.Update(this, m_cb_data.PrivateData, &cfg);
}

void CustomItem::ContextMenu(QMenu& m)
{
    LayoutItem::ContextMenu(m);
    if (m_cb_data.ContextMenu)
        m_cb_data.ContextMenu(this, m_cb_data.PrivateData, &m);
}

void CustomItem::MouseEvent(MouseData const& e, DurchblickItemConfig const& cfg)
{
    LayoutItem::MouseEvent(e, cfg);
    if (m_cb_data.MouseEvent)
        m_cb_data.MouseEvent(this, m_cb_data.PrivateData, &cfg, e.x, e.y, e.buttons, e.modifiers);
}

void CustomItem::Render(DurchblickItemConfig const& cfg)
{
    LayoutItem::Render(cfg);
    m_cb_data.Render(this, m_cb_data.PrivateData, &cfg);
}

void CustomItem::WriteToJson(QJsonObject& Obj)
{
    if (m_cb_data.Save) {
        auto* json = m_cb_data.Save(this, m_cb_data.PrivateData);
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
                m_cb_data.Name.c_str(), qt_to_utf8(err.errorString()));
        }

        free(c);
        json_decref(json);
    }
}

void CustomItem::ReadFromJson(const QJsonObject& Obj)
{
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
                m_cb_data.Load(this, m_cb_data.PrivateData, json);
                json_decref(json);
            } else {
                berr("Failed to load custom widget data for '%s' error (line %i, col %i): %s",
                    m_cb_data.Name.c_str(), err.line, err.column, err.text);
            }
        }
    }
}

uint32_t CustomItem::GetFillColor()
{
    if (m_cb_data.GetFillColor)
        return m_cb_data.GetFillColor(this, m_cb_data.PrivateData);
    return LayoutItem::GetFillColor();
}
