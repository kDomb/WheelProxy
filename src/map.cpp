/*
   Copyright (C) 2017 Kai Dombrowe <just89@gmx.de>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include "map.h"
#include "eventsource.h"


namespace WP {



Map::Map()
{

}


Map::~Map()
{

    for (auto it = m_data.begin(), end = m_data.end(); it != end; ++it) {
        auto vec = (*it).second;
        for (size_t i = 0, s = vec->size(); i < s; ++i) {
            delete (*vec)[i];
        }
        delete vec;
    }

}


void
Map::add(WP::MapEntry *entry)
{

    auto vec = get_entries_for_src(entry->get_src());
    if (vec == nullptr) {
        vec = new std::vector<WP::MapEntry*>();
        m_data.insert(std::make_pair(entry->get_src(), vec));
    }
    vec->push_back(entry);

}


std::vector<WP::MapEntry*> *
Map::get_entries_for_src(WP::EventSource *src) const
{

    const auto it = m_data.find(src);
    if (it == m_data.end()) return nullptr;
    return (*it).second;

}



} // namespace WP
