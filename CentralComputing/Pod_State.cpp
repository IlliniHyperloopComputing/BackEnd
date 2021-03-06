#include "Pod_State.h" 
#include "Pod.h"

using Utils::print;
using Utils::LogLevel;

Pod_State::Pod_State()
  : StateMachine(ST_MAX_STATES),
  motor(), brakes() {
  transition_map[Command::TRANS_SAFE_MODE] = &Pod_State::move_safe_mode;  
  transition_map[Command::TRANS_FUNCTIONAL_TEST_OUTSIDE] = &Pod_State::move_functional_tests_outside;
  transition_map[Command::TRANS_LOADING] = &Pod_State::move_loading;
  transition_map[Command::TRANS_FUNCTIONAL_TEST_INSIDE] = &Pod_State::move_functional_tests_inside;
  transition_map[Command::TRANS_LAUNCH_READY] = &Pod_State::move_launch_ready;
  transition_map[Command::TRANS_FLIGHT_ACCEL] = &Pod_State::accelerate;
  transition_map[Command::TRANS_FLIGHT_COAST] = &Pod_State::coast;
  transition_map[Command::TRANS_FLIGHT_BRAKE] = &Pod_State::brake;
  transition_map[Command::TRANS_ABORT]= &Pod_State::move_safe_mode_or_abort;
  // non state transition commands
  transition_map[Command::ENABLE_MOTOR] = &Pod_State::no_transition;
  transition_map[Command::DISABLE_MOTOR] = &Pod_State::no_transition;
  transition_map[Command::SET_MOTOR_SPEED] = &Pod_State::no_transition;
  transition_map[Command::ENABLE_BRAKE] = &Pod_State::no_transition;
  transition_map[Command::DISABLE_BRAKE] = &Pod_State::no_transition;
  transition_map[Command::SET_ADC_ERROR] = &Pod_State::move_safe_mode_or_abort;
  transition_map[Command::SET_CAN_ERROR] = &Pod_State::move_safe_mode_or_abort;
  transition_map[Command::SET_I2C_ERROR] = &Pod_State::move_safe_mode_or_abort;
  transition_map[Command::SET_PRU_ERROR] = &Pod_State::move_safe_mode_or_abort;
  transition_map[Command::SET_NETWORK_ERROR] = &Pod_State::move_safe_mode_or_abort;
  transition_map[Command::SET_OTHER_ERROR] = &Pod_State::move_safe_mode_or_abort;
  transition_map[Command::CLR_ADC_ERROR] = &Pod_State::no_transition;
  transition_map[Command::CLR_CAN_ERROR] = &Pod_State::no_transition;
  transition_map[Command::CLR_I2C_ERROR] = &Pod_State::no_transition;
  transition_map[Command::CLR_PRU_ERROR] = &Pod_State::no_transition;
  transition_map[Command::CLR_NETWORK_ERROR] = &Pod_State::no_transition;
  transition_map[Command::CLR_OTHER_ERROR] = &Pod_State::no_transition;
  transition_map[Command::SET_HV_RELAY_HV_POLE] = &Pod_State::no_transition;
  transition_map[Command::SET_HV_RELAY_LV_POLE] = &Pod_State::no_transition;
  transition_map[Command::SET_HV_RELAY_PRE_CHARGE] = &Pod_State::no_transition;
  transition_map[Command::CALC_ACCEL_ZERO_G] = &Pod_State::no_transition;
  transition_map[Command::RESET_PRU] = &Pod_State::no_transition;
  steady_state_map[ST_SAFE_MODE] = &Pod_State::steady_safe_mode;
  steady_state_map[ST_FUNCTIONAL_TEST_OUTSIDE] = &Pod_State::steady_function_outside;
  steady_state_map[ST_LOADING] = &Pod_State::steady_loading;
  steady_state_map[ST_FUNCTIONAL_TEST_INSIDE] = &Pod_State::steady_function_inside;
  steady_state_map[ST_LAUNCH_READY] = &Pod_State::steady_launch_ready;
  steady_state_map[ST_FLIGHT_ACCEL] = &Pod_State::steady_flight_accelerate;
  steady_state_map[ST_FLIGHT_COAST] = &Pod_State::steady_flight_coast;
  steady_state_map[ST_FLIGHT_BRAKE] = &Pod_State::steady_flight_brake;
  steady_state_map[ST_FLIGHT_ABORT] = &Pod_State::steady_flight_abort;

  if (!(ConfiguratorManager::config.getValue("acceleration_timeout", acceleration_timeout) && 
      ConfiguratorManager::config.getValue("precharge_timeout", launch_ready_precharge_timeout) &&
      ConfiguratorManager::config.getValue("coast_timeout", coast_timeout) &&
      ConfiguratorManager::config.getValue("brake_timeout", brake_timeout) &&
      ConfiguratorManager::config.getValue("estimated_brake_deceleration", estimated_brake_deceleration) &&
      ConfiguratorManager::config.getValue("length_of_track", length_of_track) &&
      ConfiguratorManager::config.getValue("brake_buffer_length", brake_buffer_length) &&
      ConfiguratorManager::config.getValue("not_moving_velocity", not_moving_velocity) &&
      ConfiguratorManager::config.getValue("not_moving_acceleration", not_moving_acceleration))) {
    print(LogLevel::LOG_ERROR, "CONFIG FILE ERROR: POD_STATE: Missing necessary configuration\n");
    exit(1);
  }
  p_counter = 0;
  a_counter = 0;
  c_counter = 0;
  b_counter = 0;
}

// returns the current state as a E_States enum
E_States Pod_State::get_current_state() {
  return (E_States)StateMachine::getCurrentState();
}

// Super lazy function. This isn't that great
void Pod_State::get_time_and_timeouts(int64_t *  p_elapsed_time, int64_t* p_timeout,
                            int64_t * a_elapsed_time, int64_t* a_timeout,
                            int64_t * c_elapsed_time, int64_t *c_timeout,
                            int64_t * b_elapsed_time, int64_t *b_timeout) {
  *p_timeout = launch_ready_precharge_timeout;
  *a_timeout = acceleration_timeout;
  *c_timeout = coast_timeout;
  *b_timeout = brake_timeout;
   
  std::lock_guard<std::mutex> guard(timeout_mutex);
  *p_elapsed_time = p_counter; 
  *a_elapsed_time = a_counter; 
  *c_elapsed_time = c_counter; 
  *b_elapsed_time = b_counter; 
}

/**
 * User controlled movement events
 * For each TRANSITION_MAP_ENTRY(_STATE_), 
 * We can enter the _STATE_ from the commemented state on the right
**/
/**
 * Can enter (transition into) SAFE_MODE from: 
 * --Safe Mode (Why?? To re-trigger the safing actions that ST_Safe_Mode() does)
 * --Functional test
 * --Loading
 * --Launch Ready
 * --Flight Brake
 **/
void Pod_State::move_safe_mode() {
  BEGIN_TRANSITION_MAP              /* Current state */
    TRANSITION_MAP_ENTRY(ST_SAFE_MODE)     /* Safe Mode */
    TRANSITION_MAP_ENTRY(ST_SAFE_MODE)      /* Functional test outside*/
    TRANSITION_MAP_ENTRY(ST_SAFE_MODE)      /* Loading */
    TRANSITION_MAP_ENTRY(ST_SAFE_MODE)      /* Functional test inside*/
    TRANSITION_MAP_ENTRY(ST_SAFE_MODE)      /* Launch ready */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Flight accel */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Flight coast */
    TRANSITION_MAP_ENTRY(ST_SAFE_MODE)      /* Flight brake */
    TRANSITION_MAP_ENTRY(ST_SAFE_MODE)     /* Flight Abort */
  END_TRANSITION_MAP(NULL)
}

/**
 * Can enter (transition into) FUNCTIONAL_TEST_OUTSIDE only from Safe Mode
 **/
void Pod_State::move_functional_tests_outside() {
  BEGIN_TRANSITION_MAP                      /* Current state */
    TRANSITION_MAP_ENTRY(ST_FUNCTIONAL_TEST_OUTSIDE)      /* Safe Mode */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Functional test */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Loading */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Functional test inside*/
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Launch ready */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Flight accel */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Flight coast */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Flight brake */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Flight Abort */
  END_TRANSITION_MAP(NULL)
}

void Pod_State::move_loading() {
  BEGIN_TRANSITION_MAP                      /* Current state */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Safe Mode */
    TRANSITION_MAP_ENTRY(ST_LOADING)        /* Functional test */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Loading */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Functional test inside*/
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Launch ready */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Flight accel */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Flight coast */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Flight brake */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Flight Abort */
  END_TRANSITION_MAP(NULL)
}

void Pod_State::move_functional_tests_inside() {
  BEGIN_TRANSITION_MAP                      /* Current state */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Safe Mode */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Functional test outside */
    TRANSITION_MAP_ENTRY(ST_FUNCTIONAL_TEST_INSIDE) /* Loading */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Functional test inside */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Launch ready */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Flight accel */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Flight coast */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Flight brake */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Flight Abort */
  END_TRANSITION_MAP(NULL)
}

void Pod_State::move_launch_ready() {
  BEGIN_TRANSITION_MAP                      /* Current state */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Safe Mode */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Functional test outside */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Loading */
    TRANSITION_MAP_ENTRY(ST_LAUNCH_READY)   /* Functional test inside */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Launch ready */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Flight accel */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Flight coast */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Flight brake */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Flight Abort */
  END_TRANSITION_MAP(NULL)

  auto_transition_safe_mode.reset();
  auto_transition_brake.reset();
  auto_transition_coast.reset();
  auto_transition_safe_mode.reset();
}

/**
 * Software controlled events
 **/
void Pod_State::accelerate() {
  BEGIN_TRANSITION_MAP                      /* Current state */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Safe Mode */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Functional test outside */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Loading */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Functional test inside */
    TRANSITION_MAP_ENTRY(ST_FLIGHT_ACCEL)   /* Launch ready */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Flight accel */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Flight coast */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Flight brake */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Flight Abort */
  END_TRANSITION_MAP(NULL)
}

void Pod_State::coast() {
  BEGIN_TRANSITION_MAP                      /* Current state */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Safe Mode */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Functional test outside */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Loading */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Functional test inside */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Launch ready */
    TRANSITION_MAP_ENTRY(ST_FLIGHT_COAST)   /* Flight accel */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Flight coast */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Flight brake */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Flight Abort */
  END_TRANSITION_MAP(NULL)
}

void Pod_State::brake() {
  BEGIN_TRANSITION_MAP                      /* Current state */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Safe Mode */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Functional test outside */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Loading */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Functional test inside */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Launch ready */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Flight accel */
    TRANSITION_MAP_ENTRY(ST_FLIGHT_BRAKE)   /* Flight coast */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Flight brake */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Flight Abort */
  END_TRANSITION_MAP(NULL)
}

void Pod_State::move_safe_mode_or_abort() {
  BEGIN_TRANSITION_MAP                      /* Current state */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Safe Mode */
    TRANSITION_MAP_ENTRY(ST_SAFE_MODE)      /* Functional test outside */
    TRANSITION_MAP_ENTRY(ST_SAFE_MODE)      /* Loading */
    TRANSITION_MAP_ENTRY(ST_SAFE_MODE)      /* Functional test inside */
    TRANSITION_MAP_ENTRY(ST_SAFE_MODE)      /* Launch ready */
    TRANSITION_MAP_ENTRY(ST_FLIGHT_ABORT)   /* Flight accel */
    TRANSITION_MAP_ENTRY(ST_FLIGHT_ABORT)   /* Flight coast */
    TRANSITION_MAP_ENTRY(ST_FLIGHT_ABORT)   /* Flight brake */
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)     /* Flight Abort */
  END_TRANSITION_MAP(NULL)
}

void Pod_State::no_transition() {
  // do nothing
}

void Pod_State::ST_Safe_Mode() {
  print(LogLevel::LOG_EDEBUG, "STATE : %s\n", get_current_state_string().c_str());
  brakes.disable_brakes(); 
  motor.disable_motors();
  motor.set_relay_state(HV_Relay_Select::RELAY_LV_POLE, HV_Relay_State::RELAY_OFF);
  motor.set_relay_state(HV_Relay_Select::RELAY_HV_POLE, HV_Relay_State::RELAY_OFF);
  motor.set_relay_state(HV_Relay_Select::RELAY_PRE_CHARGE, HV_Relay_State::RELAY_OFF);
}

void Pod_State::ST_Functional_Test_Outside() {
  print(LogLevel::LOG_EDEBUG, "STATE : %s\n", get_current_state_string().c_str());
  brakes.disable_brakes();  
  motor.disable_motors();
  motor.set_relay_state(HV_Relay_Select::RELAY_LV_POLE, HV_Relay_State::RELAY_OFF);
  motor.set_relay_state(HV_Relay_Select::RELAY_HV_POLE, HV_Relay_State::RELAY_OFF);
  motor.set_relay_state(HV_Relay_Select::RELAY_PRE_CHARGE, HV_Relay_State::RELAY_OFF);
}
void Pod_State::ST_Loading() {
  print(LogLevel::LOG_EDEBUG, "STATE : %s\n", get_current_state_string().c_str());
  brakes.disable_brakes();  
  motor.disable_motors();
  motor.set_relay_state(HV_Relay_Select::RELAY_LV_POLE, HV_Relay_State::RELAY_OFF);
  motor.set_relay_state(HV_Relay_Select::RELAY_HV_POLE, HV_Relay_State::RELAY_OFF);
  motor.set_relay_state(HV_Relay_Select::RELAY_PRE_CHARGE, HV_Relay_State::RELAY_OFF);
}
void Pod_State::ST_Functional_Test_Inside() {
  print(LogLevel::LOG_EDEBUG, "STATE : %s\n", get_current_state_string().c_str());
  brakes.disable_brakes();  
  motor.disable_motors();
  motor.set_relay_state(HV_Relay_Select::RELAY_LV_POLE, HV_Relay_State::RELAY_OFF);
  motor.set_relay_state(HV_Relay_Select::RELAY_HV_POLE, HV_Relay_State::RELAY_OFF);
  motor.set_relay_state(HV_Relay_Select::RELAY_PRE_CHARGE, HV_Relay_State::RELAY_OFF);
}
void Pod_State::ST_Launch_Ready() {
  print(LogLevel::LOG_EDEBUG, "STATE : %s\n", get_current_state_string().c_str());
  launch_ready_start_time = microseconds();
  ready_for_launch = false;  // used in conjunction with the launch_ready_start_time timer.

  brakes.disable_brakes();
  motor.enable_motors();

  // NOTE: We turn on the Precharge and LV Relays. 
  motor.set_relay_state(HV_Relay_Select::RELAY_PRE_CHARGE, HV_Relay_State::RELAY_ON);
  motor.set_relay_state(HV_Relay_Select::RELAY_LV_POLE, HV_Relay_State::RELAY_ON);
  launch_ready_start_time = microseconds();
}

void Pod_State::ST_Flight_Accel() {
  print(LogLevel::LOG_EDEBUG, "STATE : %s\n", get_current_state_string().c_str());
  flight_plan_index = 0;
  old_motor_throttle = -1;
  acceleration_start_time = microseconds();
}

void Pod_State::ST_Flight_Coast() {
  print(LogLevel::LOG_EDEBUG, "STATE : %s\n", get_current_state_string().c_str());
  coast_start_time = microseconds();
  motor.disable_motors();
  motor.set_relay_state(HV_Relay_Select::RELAY_LV_POLE, HV_Relay_State::RELAY_OFF);
  motor.set_relay_state(HV_Relay_Select::RELAY_HV_POLE, HV_Relay_State::RELAY_OFF);
  motor.set_relay_state(HV_Relay_Select::RELAY_PRE_CHARGE, HV_Relay_State::RELAY_OFF);
}

void Pod_State::ST_Flight_Brake() {
  print(LogLevel::LOG_EDEBUG, "STATE : %s\n", get_current_state_string().c_str());
  motor.disable_motors();
  motor.set_relay_state(HV_Relay_Select::RELAY_LV_POLE, HV_Relay_State::RELAY_OFF);
  motor.set_relay_state(HV_Relay_Select::RELAY_HV_POLE, HV_Relay_State::RELAY_OFF);
  motor.set_relay_state(HV_Relay_Select::RELAY_PRE_CHARGE, HV_Relay_State::RELAY_OFF);
  brakes.enable_brakes();
  brake_start_time = microseconds();
}

void Pod_State::ST_Flight_Abort() {
  print(LogLevel::LOG_EDEBUG, "STATE : %s\n", get_current_state_string().c_str());
  motor.disable_motors();
  motor.set_relay_state(HV_Relay_Select::RELAY_LV_POLE, HV_Relay_State::RELAY_OFF);
  motor.set_relay_state(HV_Relay_Select::RELAY_HV_POLE, HV_Relay_State::RELAY_OFF);
  motor.set_relay_state(HV_Relay_Select::RELAY_PRE_CHARGE, HV_Relay_State::RELAY_OFF);
  brakes.enable_brakes();
}

/////////////////////////////
// STEADY STATE FUNCTIONS //
///////////////////////////
void Pod_State::steady_safe_mode(Command::Network_Command * command, 
                                  UnifiedState * state) {
  switch (command->id) {
    case Command::CALC_ACCEL_ZERO_G:
      // trigger calculate zero g
      SourceManager::ADC.calculate_zero_g();
      break;
    default:
      break;
  }
}

void Pod_State::steady_function_outside(Command::Network_Command * command, 
                                  UnifiedState * state) {
  // process command, let manual commands go through
  switch (command->id) {
    case Command::ENABLE_MOTOR: 
      motor.enable_motors();
      break;
    case Command::DISABLE_MOTOR:
      motor.disable_motors();
      break;
    case Command::SET_MOTOR_SPEED:
      motor.set_throttle(command->value); 
      break;
    case Command::SET_HV_RELAY_HV_POLE:
      if (command->value == 0) {
        motor.set_relay_state(HV_Relay_Select::RELAY_HV_POLE, HV_Relay_State::RELAY_OFF);
      } else if (command->value == 1) {
        motor.set_relay_state(HV_Relay_Select::RELAY_HV_POLE, HV_Relay_State::RELAY_ON);
      }
      break;
    case Command::SET_HV_RELAY_LV_POLE:
      if (command->value == 0) {
        motor.set_relay_state(HV_Relay_Select::RELAY_LV_POLE, HV_Relay_State::RELAY_OFF);
      } else if (command->value == 1) {
        motor.set_relay_state(HV_Relay_Select::RELAY_LV_POLE, HV_Relay_State::RELAY_ON);
      }
      break;
    case Command::SET_HV_RELAY_PRE_CHARGE:
      if (command->value == 0) {
        motor.set_relay_state(HV_Relay_Select::RELAY_PRE_CHARGE, HV_Relay_State::RELAY_OFF);
      } else if (command->value == 1) {
        motor.set_relay_state(HV_Relay_Select::RELAY_PRE_CHARGE, HV_Relay_State::RELAY_ON);
      }
      break;
    case Command::ENABLE_BRAKE:
      // activate brakes
      brakes.enable_brakes();
      break;
    case Command::DISABLE_BRAKE:
      // deactivate brakes
      brakes.disable_brakes();
      break;
    case Command::CALC_ACCEL_ZERO_G:
      // trigger calculate zero g
      SourceManager::ADC.calculate_zero_g();
      break;
    case Command::RESET_PRU:
      break;
    default:
      break;
  }
}

void Pod_State::steady_loading(Command::Network_Command * command, 
                                UnifiedState* state) {
  switch (command->id) {
    case Command::CALC_ACCEL_ZERO_G:
      // trigger calculate zero g
      SourceManager::ADC.calculate_zero_g();
      break;
    default:
      break;
  }
}

void Pod_State::steady_function_inside(Command::Network_Command * command, 
                                  UnifiedState * state) {
  switch (command->id) {
    case Command::ENABLE_MOTOR: 
      motor.enable_motors();
      break;
    case Command::DISABLE_MOTOR:
      motor.disable_motors();
      break;
    case Command::SET_MOTOR_SPEED:
      // print out something?
      break;
    case Command::SET_HV_RELAY_HV_POLE:
      if (command->value == 0) {
        motor.set_relay_state(HV_Relay_Select::RELAY_HV_POLE, HV_Relay_State::RELAY_OFF);
      } else if (command->value == 1) {
        motor.set_relay_state(HV_Relay_Select::RELAY_HV_POLE, HV_Relay_State::RELAY_ON);
      }
      break;
    case Command::SET_HV_RELAY_LV_POLE:
      if (command->value == 0) {
        motor.set_relay_state(HV_Relay_Select::RELAY_LV_POLE, HV_Relay_State::RELAY_OFF);
      } else if (command->value == 1) {
        motor.set_relay_state(HV_Relay_Select::RELAY_LV_POLE, HV_Relay_State::RELAY_ON);
      }
      break;
    case Command::SET_HV_RELAY_PRE_CHARGE:
      if (command->value == 0) {
        motor.set_relay_state(HV_Relay_Select::RELAY_PRE_CHARGE, HV_Relay_State::RELAY_OFF);
      } else if (command->value == 1) {
        motor.set_relay_state(HV_Relay_Select::RELAY_PRE_CHARGE, HV_Relay_State::RELAY_ON);
      }
      break;
    case Command::CALC_ACCEL_ZERO_G:
      // trigger calculate zero g
      SourceManager::ADC.calculate_zero_g();
      break;
    default:
      break;
  }
}

void Pod_State::steady_launch_ready(Command::Network_Command * command, 
                                    UnifiedState* state) {
  switch (command->id) {
    case Command::RESET_PRU:
      break;
    default:
      break;
  }
  // check if precharge complete
  int64_t timeout_check = microseconds() - launch_ready_start_time;
  if (timeout_check > launch_ready_precharge_timeout && !ready_for_launch) {
    // Precharge complete, turn of precharge relay, turn on HV relay
    motor.set_relay_state(HV_Relay_Select::RELAY_PRE_CHARGE, HV_Relay_State::RELAY_OFF);
    motor.set_relay_state(HV_Relay_Select::RELAY_HV_POLE, HV_Relay_State::RELAY_ON);
    ready_for_launch = true;  // Set true so we can't get into this IF again
  }

  std::lock_guard<std::mutex> guard(timeout_mutex);
  p_counter = timeout_check;
}

void Pod_State::steady_flight_accelerate(Command::Network_Command * command, 
                                        UnifiedState* state) {
  // Access Pos, Vel, and Accel from Motion Model
  int32_t pos = state->motion_data->x[0];
  int32_t vel = state->motion_data->x[1];
  int32_t acc = state->motion_data->x[2];
  int64_t timeout_check = microseconds() - acceleration_start_time;

  int16_t motor_throttle = ConfiguratorManager::config.getFlightPlan(timeout_check, &flight_plan_index);
  if (motor_throttle != old_motor_throttle) {
    old_motor_throttle = motor_throttle;
    motor.set_throttle(motor_throttle);
  }
  
  // Transition if kinematics demand it, or we exceed our timeout
  if (shouldBrake(vel, pos) || timeout_check >= acceleration_timeout) {
    Command::put(Command::Network_Command_ID::TRANS_FLIGHT_COAST, 0);
    auto_transition_coast.invoke();
  }

  std::lock_guard<std::mutex> guard(timeout_mutex);
  a_counter = timeout_check;
}

void Pod_State::steady_flight_coast(Command::Network_Command * command, 
                                    UnifiedState* state) {
  // Transition after we exceed our timeout
  int64_t timeout_check = microseconds() - coast_start_time;
  if (timeout_check >= coast_timeout) {
    Command::put(Command::Network_Command_ID::TRANS_FLIGHT_BRAKE, 0);
    auto_transition_brake.invoke();
  }

  std::lock_guard<std::mutex> guard(timeout_mutex);
  c_counter = timeout_check;
}

void Pod_State::steady_flight_brake(Command::Network_Command * command, 
                                    UnifiedState* state) {
  int32_t acc = state->motion_data->x[2];
  int32_t vel = state->motion_data->x[1];
  int64_t timeout_check = microseconds() - brake_start_time;

  // Transition after we exceed our timeout AND acceleration AND Velocity are under a configurable value.
  if (std::abs(acc) < not_moving_acceleration
      && std::abs(vel) < not_moving_velocity 
      && timeout_check >= brake_timeout) {
    Command::put(Command::Network_Command_ID::TRANS_SAFE_MODE, 0);
    auto_transition_safe_mode.invoke();
  }

  std::lock_guard<std::mutex> guard(timeout_mutex);
  b_counter = timeout_check;
}

bool Pod_State::shouldBrake(int64_t vel, int64_t pos) {
  int64_t target_distance = length_of_track - brake_buffer_length;
  int64_t stopping_distance = pos + (vel * vel) / (2*estimated_brake_deceleration);

  if (stopping_distance >= target_distance) {
    print(LogLevel::LOG_INFO, "Pod Should Brake, vel: %.2f pos: %.2f\n", vel, pos);
    return true;
  } else {
    return false;
  }
}

void Pod_State::steady_flight_abort(Command::Network_Command * command, 
                                    UnifiedState * state) {
}
