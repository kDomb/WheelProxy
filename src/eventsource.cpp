#include "eventsource.h"

namespace WP {

EventSource::EventSource(Type type)
{

    m_type = type;
    m_code = 0;

}


EventSource::~EventSource()
{


}


EventSource::Type
EventSource::get_type() const
{

    return m_type;

}


uint16_t
EventSource::get_code() const
{

    return m_code;

}


void
EventSource::set_code(uint16_t code)
{

    m_code = code;

}


std::string
EventSource::get_name() const
{

    return m_name;

}


void
EventSource::set_name(const std::string &name)
{

    m_name = name;

}



} // namespace WP
