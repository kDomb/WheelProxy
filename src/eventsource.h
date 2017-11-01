#ifndef EVENTSOURCE_H
#define EVENTSOURCE_H


#include <stdint.h>
#include <string>


namespace WP {



class EventSource
{


public:
    enum Type {
        TYPE_BUTTON,
        TYPE_AXIS
    };


    EventSource(Type type);
    virtual ~EventSource();

    Type get_type() const;

    uint16_t get_code() const;
    void set_code(uint16_t code);

    std::string get_name() const;
    void set_name(const std::string &name);


private:
    Type m_type;
    std::string m_name;
    uint16_t m_code;


};



} // namespace WP



#endif // EVENTSOURCE_H
