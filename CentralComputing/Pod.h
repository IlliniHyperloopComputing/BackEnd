#ifndef POD_H
#define POD_H

#include "NetworkManager.hpp"
#include "ParameterManager.hpp"
#include "Event.hpp"
#include "Pod_State.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wctor-dtor-privacy"
#include "gtest/gtest.h"
#include <semaphore.h>
#pragma GCC diagnostic pop


class Pod {
  public:
    Pod();

    void startup();

    void stop();

    shared_ptr<Pod_State> state_machine;
    atomic<bool> running;
    Event ready;
    Event processing_command;
    Event closing;

  private:
    void logic_loop();
};

void signal_handler(int signal);


#endif
