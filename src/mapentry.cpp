#include "mapentry.h"


#include <assert.h>



namespace WP {


MapEntry::Data::Data()
{


}


MapEntry::Data::~Data()
{


}



AxisToButtonData::AxisToButtonData(int32_t start, int32_t end)
    : MapEntry::Data()
{

    m_range[0] = start;
    m_range[1] = end;

    assert(end >= start);

}


AxisToButtonData::~AxisToButtonData()
{


}



int32_t
AxisToButtonData::get_range_start() const
{

    return m_range[0];

}


int32_t
AxisToButtonData::get_range_end() const
{

    return m_range[1];

}



ButtonToAxisData::ButtonToAxisData(int32_t value_released, int32_t value_pressed)
    : MapEntry::Data()
{

    m_values[0] = value_released;
    m_values[1] = value_pressed;

}


ButtonToAxisData::~ButtonToAxisData()
{


}


int32_t
ButtonToAxisData::get_value_released() const
{

    return m_values[0];

}


int32_t
ButtonToAxisData::get_value_pressed() const
{

    return m_values[1];

}



MapEntry::MapEntry(WP::EventSource *src, WP::EventSource *target, WP::MapEntry::Data *data)
{

    m_data = data;
    m_src = src;
    m_target = target;

    assert(src != nullptr);
    assert(target != nullptr);

}


MapEntry::~MapEntry()
{

    delete m_data;

}



WP::MapEntry::Data *MapEntry::get_data() const
{

    return m_data;

}


WP::EventSource *MapEntry::get_src() const
{

    return m_src;

}


WP::EventSource *MapEntry::get_target() const
{

    return m_target;

}



} // namespace WP
