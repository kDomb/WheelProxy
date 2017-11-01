#ifndef MAPENTRY_H
#define MAPENTRY_H


#include <stdint.h>


namespace WP {



class EventSource;
class MapEntry
{


public:
    class Data {
    public:
        Data();
        virtual ~Data();
    };


    MapEntry(WP::EventSource *src, WP::EventSource *target, WP::MapEntry::Data *data);
    ~MapEntry();


    WP::MapEntry::Data *get_data() const;
    WP::EventSource *get_src() const;
    WP::EventSource *get_target() const;


private:
    WP::MapEntry::Data *m_data;
    WP::EventSource *m_src;
    WP::EventSource *m_target;


};


class AxisToButtonData : public MapEntry::Data
{


public:
    AxisToButtonData(int32_t start, int32_t end);
    ~AxisToButtonData();

    int32_t get_range_start() const;
    int32_t get_range_end() const;


private:
    int32_t m_range[2];


};


class ButtonToAxisData : public MapEntry::Data {


public:
    ButtonToAxisData(int32_t value_released, int32_t value_pressed);
    ~ButtonToAxisData();

    int32_t get_value_released() const;
    int32_t get_value_pressed() const;


private:
    int32_t m_values[2];


};


} // namespace WP



#endif // MAPENTRY_H
