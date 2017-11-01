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


#ifndef UTILS_H
#define UTILS_H


#include <stdlib.h>
#include <iostream>

namespace WP {



class Utils
{


public:
    template <typename T>
    static void deleteAll(T &cont) {
        for (size_t i = 0, s = cont.size(); i < s; ++i) delete cont[i];
        cont.clear();
    }


};


} // namespace WP


#define DELETE_ALL(c) WP::Utils::deleteAll(c)

#endif // UTILS_H
