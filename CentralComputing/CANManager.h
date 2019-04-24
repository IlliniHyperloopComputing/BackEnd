#ifndef CANMANAGER_H_
#define CANMANAGER_H_

#include "SourceManagerBase.hpp"
#include "Defines.hpp"

class CANManager : public SourceManagerBase<CANData, false> {
 private:
  bool initialize_source();
  void stop_source();
  std::shared_ptr<CANData> refresh();
  std::shared_ptr<CANData> refresh_sim();

  std::string name() {
    return "can";
  }

  // iterator for testing purposes, remove
  int i = 5;
};

#endif  // CANMANAGER_H_
