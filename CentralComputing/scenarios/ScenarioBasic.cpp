#ifdef SIM
#include "ScenarioBasic.h"

#define MAX_ACCEL 9.81
#define MAX_DECEL -9.81

bool ScenarioBasic::use_motion_model() {
  return true;
}

std::shared_ptr<MotionData> ScenarioBasic::sim_get_motion() {
  // FOR FIRST CALL
  if (timeLast == -1) {
    timeDelta = 0.000;
  } else {  // SUBSEQUENT CALLS
    timeDelta = Utils::microseconds() - timeLast;
  }

  if (motorsOn) {
    acceleration = MAX_ACCEL * throttle;
  } else if (brakesOn) {
    acceleration = MAX_DECEL * pressure;
  } else {
    acceleration = 0;
  }


  double deltaSeconds = static_cast<double>(timeDelta) / 1000000.0;

  // KINEMATIC PHYSICS CALCULATIONS
  velocity = lastVelocity + (acceleration * deltaSeconds);
  position = lastPosition + ((lastVelocity + velocity)/2 * deltaSeconds) 
              + (0.5 * acceleration * deltaSeconds * deltaSeconds);

  // CREATING A STATESPACE OBJECT AND SETTING ITS ARRAY'S VALUES
  std::shared_ptr<MotionData> space = std::make_shared<MotionData>();
  space->x[0] = position;
  space->x[1] = velocity;
  space->x[2] = acceleration;

  if (enable_logging) {
    print(LogLevel::LOG_DEBUG, 
      "Motion: Pos: %.2f, Vel: %.2f, Acl= %.2f, lastPos= %.2f, lastVel = %.2f, delta = %d, timeLast=%ld, t = %ld\n",
      position, velocity, acceleration, lastPosition, lastVelocity, timeDelta, timeLast, Utils::microseconds());
  }

  // UPDATING VARIABLES
  lastPosition = position;
  lastVelocity = velocity;
  timeLast = Utils::microseconds();

  return space;
}
#endif