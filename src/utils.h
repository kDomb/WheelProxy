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
