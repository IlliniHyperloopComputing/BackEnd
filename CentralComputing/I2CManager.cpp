#include "I2CManager.h"

bool I2CManager::initialize_source() {
  print(LogLevel::LOG_DEBUG, "I2C Manger setup successful\n");
  return true;
}

void I2CManager::stop_source() {
  print(LogLevel::LOG_DEBUG, "I2C Manger stopped\n");
}

std::shared_ptr<I2CData> I2CManager::refresh() {
  std::shared_ptr<I2CData> new_data = std::make_shared<I2CData>();
  new_data->dummy_data = i;
  i++;
  return new_data;
}

std::shared_ptr<I2CData> I2CManager::refresh_sim() {
  #ifdef SIM
  return SimulatorManager::sim.sim_get_i2c();
  #else
  return empty_data();
  #endif
}

