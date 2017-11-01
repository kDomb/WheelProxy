#include "application.h"

static bool g_wp_verbose = false;



namespace WP {


bool
Application::get_verbose()
{

    return g_wp_verbose;

}


void
Application::set_verbose(bool verbose)
{

    g_wp_verbose = verbose;

}


} // namespace WP
