#ifndef COMMAND_H_
#define COMMAND_H_

#include "Utils.h"
#include "SafeQueue.hpp"
#include "Event.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <poll.h>
#include <atomic>
#include <thread> // NOLINT
#include <memory>
#include <sys/ioctl.h>

namespace Command {

extern SafeQueue<uint16_t> command_queue;
struct Network_Command;
void put(uint8_t id, uint8_t value);
bool get(Network_Command * com);
  
enum Network_Command_ID {
  // state transitions
  TRANS_SAFE_MODE = 0,
  TRANS_FUNCTIONAL_TEST = 1,
  TRANS_LOADING = 2,
  TRANS_LAUNCH_READY = 3,
  LAUNCH = 4,
  EMERGENCY_BRAKE = 5,
  ENABLE_MOTOR = 6,
  DISABLE_MOTOR = 7,
  SET_MOTOR_SPEED = 8,
  ENABLE_BRAKE = 9,
  DISABLE_BRAKE = 10,
  TRANS_FLIGHT_COAST = 11,
  TRANS_FLIGHT_BRAKE = 12,
  TRANS_ERROR_STATE = 13,
};

// enum specifying what data is sent
// [1 byte Data ID][4 byte size][size byte chunk]
enum Network_Data_ID {
  POD_STATE,
  BRAKE_STATUS,
  MOTOR_STATUS,
  POSITION,
  VELOCITY,
  ACCELERATION,
  TEMPERATURE
};

struct Network_Command {
  // state transtitions
  uint8_t id;  // id is just a network command
  uint8_t value;
};
}  // namespace Command
#endif  // COMMAND_H_
