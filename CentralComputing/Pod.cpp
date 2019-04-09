#include "Pod.h"

using Utils::print;
using Utils::LogLevel;
using Utils::microseconds;
using std::make_shared;
using std::thread;
using std::function;
using std::shared_ptr;

void Pod::logic_loop() {
  #ifdef SIM  // Used to indicate to the Simulator we have processed a command
  bool command_processed = false;
  #endif

  // Start processing/pod logic
  while (running.load()) {
    auto command = TCPManager::command_queue.dequeue();
    if (command.get() != nullptr) {
      // Parse the command and call the appropriate state machine function
      auto id = (TCPManager::Network_Command_ID) command->id;
      auto transition = state_machine->get_transition_function(id);
      ((*state_machine).*(transition))(); 

      #ifdef SIM  // Used to indicate to the Simulator that we have processed a command
      command_processed = true;
      #endif
      print(LogLevel::LOG_INFO, "Command : %d %d\n", command->id, command->value);
    } else {  // Create a "do nothing" command. This will be passed into the steady state caller below
      command = make_shared<TCPManager::Network_Command>();
      command->id = 0;
      command->value = 0;
    }

    // Get current state from all of the SourceMangaers
    update_unified_state();    


    // Calls the steady state function for the current state
    // Passes in command, and current state. 
    auto func = state_machine->get_steady_function();
    ((*state_machine).*(func))(command, unified_state); 

    #ifdef BBB
    // Send the heartbeat signal to the watchdog.
    bool is_GPIO_set = Utils::set_GPIO(Utils::HEARTBEAT_GPIO, switchVal);
    if (!is_GPIO_set) {
      print(LogLevel::LOG_ERROR, "GPIO file not being accessed correctly\n");
      // TODO: Add command to command queue
    }
    switchVal = !switchVal;
    #endif

    #ifdef SIM
    // Let the simulator know we processed a command.
    if (command_processed) {
      processing_command.invoke(); 
    }
    command_processed = false;
    #endif 

    // Sleep for the given timeout
    closing.wait_for(logic_loop_timeout);
  } 
  print(LogLevel::LOG_INFO, "Exiting Pod Logic Loop\n");
}

void Pod::update_unified_state() {
  unified_state->adc_data = SourceManager::ADC.Get();
  unified_state->can_data = SourceManager::CAN.Get();
  unified_state->i2c_data = SourceManager::I2C.Get();
  unified_state->pru_data = SourceManager::PRU.Get();
  unified_state->state = state_machine->get_current_state();

  // Update motion_data
  // Pass current state into Motion Model
  #ifndef SIM
  MotionModel::calculate(unified_state);
  #else
  MotionModel::calculate_sim(unified_state);
  #endif

  // TODO: Add more things to unified state 

  TCPManager::write_queue.enqueue(unified_state);  
}

Pod::Pod() {
  // Setup "0" time. All further calls to microseconds() use this as the base time
  microseconds();

  // If we are on the BBB, run specific setup
  #ifdef BBB
  // Start up PRU
  if (system("ls /dev | grep rpmsg > /dev/null") != 0) {
    if (system("./initPRU > /dev/null") != 0) {
      print(LogLevel::LOG_ERROR, "PRU not responding\n");
      exit(1);
    }
  }
  print(LogLevel::LOG_INFO, "PRU is on\n");    

  // Set maximum CPU frequency, gotta GO F A S T  
  system("cpufreq-set -f 1000MHz");
  print(LogLevel::LOG_INFO, "CPU freq set to 1GHz\n");    
  #endif

  // Grab Network Server variables from config file
  if (!(ConfiguratorManager::config.getValue("tcp_port", tcp_port) && 
      ConfiguratorManager::config.getValue("tcp_addr", tcp_addr) &&
      ConfiguratorManager::config.getValue("udp_send_port", udp_send) &&
      ConfiguratorManager::config.getValue("udp_recv_port", udp_recv) &&
      ConfiguratorManager::config.getValue("udp_addr", udp_addr) &&
      ConfiguratorManager::config.getValue("logic_loop_timeout", logic_loop_timeout))) {
    print(LogLevel::LOG_ERROR, "CONFIG FILE ERROR: Missing necessary configuration\n");
    exit(1);
  }

  // Setup any other member variables here
  state_machine = make_shared<Pod_State>();
  unified_state = make_shared<UnifiedState>();
  unified_state->motion_data = make_shared<MotionData>();
  unified_state->adc_data = make_shared<ADCData>();
  unified_state->can_data = make_shared<CANData>();
  unified_state->i2c_data = make_shared<I2CData>();
  unified_state->pru_data = make_shared<PRUData>();
  running.store(false);
  switchVal = false;
}

void Pod::run() {
  print(LogLevel::LOG_INFO, "\n");
  print(LogLevel::LOG_INFO, "==================\n");
  print(LogLevel::LOG_INFO, "ILLINI  HYPERLOOP \n");
  print(LogLevel::LOG_INFO, "==================\n");
  print(LogLevel::LOG_INFO, "Pod State: %s\n", state_machine->get_current_state_string().c_str());

  // Start all SourceManager threads
  SourceManager::PRU.initialize();
  SourceManager::CAN.initialize();
  SourceManager::TMP.initialize();
  SourceManager::ADC.initialize();
  SourceManager::I2C.initialize();

  signal(SIGPIPE, SIG_IGN);  // Ignore SIGPIPE

  // Start Network and main loop thread.
  // I don't know how to use member functions as a thread function, but lambdas work
  thread tcp_thread([&](){ TCPManager::tcp_loop(tcp_addr.c_str(), tcp_port.c_str()); });
  thread udp_thread([&](){ UDPManager::connection_monitor(udp_addr.c_str(), udp_send.c_str(), udp_recv.c_str()); });
  running.store(true);
  thread logic_thread([&](){ logic_loop(); });  
  print(LogLevel::LOG_INFO, "Finished Initialization\n");
  print(LogLevel::LOG_INFO, "================\n\n");
  
  // signal to test suite that we are ready to begin testing
  ready.invoke();

  // Join all threads
  logic_thread.join();
  // Once logic_loop joins, trigger other threads to stop
  TCPManager::close_client();
  UDPManager::close_client();
  tcp_thread.join();
  udp_thread.join();
  SourceManager::PRU.stop();
  SourceManager::CAN.stop();
  SourceManager::TMP.stop();
  SourceManager::ADC.stop();
  SourceManager::I2C.stop();
  print(LogLevel::LOG_INFO, "All threads closed, Pod shutting down\n");
}

// Cause the logic_loop to close.
void Pod::trigger_shutdown() {
  running.store(false);  
  closing.invoke();
}

function<void(int)> shutdown_handler;
void signal_handler(int signal) {shutdown_handler(signal); }

int main(int argc, char **argv) {
  // Load the configuration file if specified, or use the default
  string config_to_open = "defaultConfig.txt";
  if (argc > 1) {  // If the first argument is a file, use it as the config file
    ifstream test_if_file(argv[1]);
    if (test_if_file.is_open()) {
      test_if_file.close();
      config_to_open = argv[1];
    }
  }
  if (!ConfiguratorManager::config.openConfigFile(config_to_open)) {
    print(LogLevel::LOG_ERROR, "Config missing. File: %s\n", config_to_open.c_str());
    return 1;
  }

  #ifndef SIM
    Utils::loglevel = LogLevel::LOG_EDEBUG;
    // Create the pod object
    auto pod = make_shared<Pod>();
    // Setup signals handlers
    signal(SIGINT, signal_handler);  // ctrl-c handler
    shutdown_handler = [&](int signal) { pod->trigger_shutdown(); };
    // Start the pod running
    pod->run();
    return 0;
  #else
    Utils::loglevel = LogLevel::LOG_EDEBUG;
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
  #endif
}
