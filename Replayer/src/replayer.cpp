/* API Interceptor (c) 2024 Dominik Witczak
 *
 * This code is licensed under MIT license (see LICENSE.txt for details)
 */
#include "replayer.h"


Replayer::Replayer()
{
    /* Stub */
}

Replayer::~Replayer()
{
    m_replayer_window_ptr.reset();
}

ReplayerUniquePtr Replayer::create()
{
    ReplayerUniquePtr result_ptr(new Replayer() );

    if (result_ptr != nullptr)
    {
        if (!result_ptr->init() )
        {
            result_ptr.reset();
        }
    }

    return result_ptr;
}

bool Replayer::init()
{
    m_replayer_window_ptr = ReplayerWindow::create();

    return true;
}