#ifndef SIM
#include "Sensor.h"
#include "Sensor_Aux/Distance.h"

Sensor::Sensor(Xmega * xm) : xmega(xm){
  //setup maps
  raw_data_map = raw_data_map_t();
  calculation_map = calculation_map_t();
  parse_map = parse_map_t();
  
  // Distance
  raw_data_map[Data_ID::DISTANCE] = *(Arbitrary_Data * ) malloc(sizeof(Distance_Raw));
  calculation_map[Data_ID::DISTANCE] = distance_calculation;
  parse_map[Data_ID::DISTANCE] = distance_parse;




}

Data Sensor::get_data(Data_ID id){
  Arbitrary_Data raw_data = get_raw_data(id);
  calculation_func_t f = calculation_map[id];
  Arbitrary_Data calculated = f(raw_data);
  Data d;
  d.calculated = calculated;
  d.raw = raw_data;
  return d;

}

Arbitrary_Data Sensor::get_raw_data(Data_ID id){
  sensor_mutex.lock();
  Arbitrary_Data raw;
  raw.size = raw_data_map[id].size;
  raw.data = (uint8_t *) malloc(raw.size);
  memcpy(raw.data, raw_data_map[id].data, raw.size);
  sensor_mutex.unlock();
  return raw;
}

void Sensor::update_buffers() {
  //update from Xmega

  for(Data_ID id : ids){
    parse_func_t fun = parse_map[id];
    Arbitrary_Data raw = raw_data_map[id];
    fun(data_buffer, raw); 
  }
}

#endif
