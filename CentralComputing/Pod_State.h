#ifndef POD_STATE_H_
#define POD_STATE_H_

#include "StateMachineCompact/StateMachine.h"
#include "Motor.h"
#include "Brakes.h"
#include "TCPManager.h"
#include "Defines.hpp"
#include <iostream>
#include <string>
#include <map>

class Pod_State;
typedef void (Pod_State::*steady_state_function) (std::shared_ptr<TCPManager::Network_Command> command, 
                                                  std::shared_ptr<UnifiedState> state);
typedef void (Pod_State::*transition_function) ();

class Pod_State : public StateMachine {
 public:
  /** 
  * Constructs a pod state machine
  **/
  Pod_State();

  // returns the current state as an enum
  E_States get_current_state();
  
  // returns the string name for current state
  std::string get_current_state_string() {
    std::string states[] = {
      "SAFE_MODE",
      "FUNCTIONAL_TESTS",
      "LOADING",
      "LAUNCH_READY",
      "FLIGHT_ACCEL",
      "FLIGHT_COAST",
      "FLIGHT_BRAKE",
      "ERROR_STATE",
      "NOT A STATE"
    };
    return states[static_cast<int>(get_current_state())];
  }
  
  /**
  * User controlled movement events
  **/
  void move_safe_mode();
  void move_functional_tests();
  void move_loading();
  void move_launch_ready();
  void accelerate();
  void move_flight_coast();
  void move_flight_brake();
  void emergency_brake();
  void no_transition();  // used in map to make things nice

  /**
  * Software controlled events
  **/
  void coast();
  void brake();
  void error();

  /**
  * Steady state functions
  * Each function call acts as a "frame"
  * Each frame, the function will proces the command, 
  **/
  void steady_safe_mode(std::shared_ptr<TCPManager::Network_Command>, std::shared_ptr<UnifiedState>);
  void steady_functional(std::shared_ptr<TCPManager::Network_Command>, std::shared_ptr<UnifiedState>);
  void steady_loading(std::shared_ptr<TCPManager::Network_Command>, std::shared_ptr<UnifiedState>);
  void steady_launch_ready(std::shared_ptr<TCPManager::Network_Command>, std::shared_ptr<UnifiedState>);
  void steady_flight_accelerate(std::shared_ptr<TCPManager::Network_Command>, std::shared_ptr<UnifiedState>);
  void steady_flight_coast(std::shared_ptr<TCPManager::Network_Command>, std::shared_ptr<UnifiedState>);
  void steady_flight_brake(std::shared_ptr<TCPManager::Network_Command>, std::shared_ptr<UnifiedState>);
  void steady_error_state(std::shared_ptr<TCPManager::Network_Command>, std::shared_ptr<UnifiedState>);

  /*
  * Gets the steady state function for the current state
  * @return a member function pointer
  */
  steady_state_function get_steady_function() {
    return steady_state_map[get_current_state()];
  }

  /*
  * Gets the transition function for the given network command
  * @return a member function pointer
  */
  transition_function get_transition_function(TCPManager::Network_Command_ID id) {
    return transition_map[id];
  }

  Motor motor;
  Brakes brakes;
  Event auto_transition_coast;
  Event auto_transition_brake;
    
 private:
  std::map<TCPManager::Network_Command_ID, transition_function> transition_map; 
  
  std::map<E_States, steady_state_function> steady_state_map;
  void ST_Safe_Mode();
  void ST_Functional_Test();
  void ST_Loading();
  void ST_Launch_Ready();
  void ST_Flight_Accel();
  void ST_Flight_Coast();
  void ST_Flight_Brake();
  void ST_Error();
  bool shouldBrake(double, double);



  BEGIN_STATE_MAP
    STATE_MAP_ENTRY(&Pod_State::ST_Safe_Mode)
    STATE_MAP_ENTRY(&Pod_State::ST_Functional_Test)
    STATE_MAP_ENTRY(&Pod_State::ST_Loading)
    STATE_MAP_ENTRY(&Pod_State::ST_Launch_Ready)
    STATE_MAP_ENTRY(&Pod_State::ST_Flight_Accel)
    STATE_MAP_ENTRY(&Pod_State::ST_Flight_Coast)
    STATE_MAP_ENTRY(&Pod_State::ST_Flight_Brake)
    STATE_MAP_ENTRY(&Pod_State::ST_Error)
  END_STATE_MAP
};


#endif  // POD_STATE_H_
