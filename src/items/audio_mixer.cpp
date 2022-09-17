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

#include "audio_mixer.hpp"

QWidget* AudioMixerItem::GetConfigWidget()
{
    return new MixerItemWidget();
}

void AudioMixerItem::LoadConfigFromWidget(QWidget*)
{
}

void AudioMixerItem::Render(const DurchblickItemConfig& cfg)
{
    LayoutItem::Render(cfg);
    m_mixer->Render(cfg.scale, 1, 1);
}

void AudioMixerItem::WriteToJson(QJsonObject& Obj)
{
    LayoutItem::WriteToJson(Obj);
}

void AudioMixerItem::ReadFromJson(const QJsonObject& Obj)
{
    LayoutItem::ReadFromJson(Obj);
}

void AudioMixerItem::Update(const DurchblickItemConfig& cfg)
{
    LayoutItem::Update(cfg);
    m_mixer->Update(cfg);
}

void AudioMixerItem::MouseEvent(const MouseData& e, const DurchblickItemConfig& cfg)
{
    LayoutItem::MouseEvent(e, cfg);
    m_mixer->MouseEvent(e, cfg);
}
