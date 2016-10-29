#include "codec.h"

command_ptr codec::decode_input(const std::string & x){
    command_ptr ep;
        //commands of the form LEV1000, FA1
        std::string state = x.substr(0,3);
        std::cout << "Command string = " << x << std::endl; 
        if(state == "SMD")
             ep = command_ptr(new command(SAFE_MODE));
        else if(state == "ISN")
             ep = command_ptr(new command(INIT_SENSORS));
        else if(state == "EMB")
             ep = command_ptr(new command(E_BRAKE));
        else if(state == "FTA")
             ep = command_ptr(new command(FUNCT_A));
        else if(state == "FTB")
             ep = command_ptr(new command(FUNCT_B));
        else if(state == "FTC")
             ep = command_ptr(new command(FUNCT_C));
        else if(state == "FTD")
             ep = command_ptr(new command(FUNCT_D));
        else if(state == "LDG")
             ep = command_ptr(new command(LOADING));
        else if(state == "FA1")
             ep = command_ptr(new command(FLIGHT_A));
        
        else if(state == "LEV"){
             int value = stoi(x.substr(3));
             ep = command_ptr(new command(LEV_MOTOR, value));
        }
        else if(state == "STA"){
             int value = stoi(x.substr(3));
             ep = command_ptr(new command(LEV_MOTOR, value)); 
        }
        else if(state == "OFF"){
            ep = command_ptr(new command(OFF));
        }
        else {
            ep = command_ptr(new command(SAFE_MODE));
        }
    return ep;
}


void codec::append_to_data_buffer(std::string & buff, const std::string & type, double * data, size_t data_length){
    std::string message;
    
    message.reserve((21 * data_length));
    for(size_t i = 0; i<data_length; i++){
        char tmp[21];
        sprintf(tmp, "%F", data[i]);
        message += tmp;
        message += ",";
    }
    //remove last comma and compute size
    int message_size = message.length();
    message = message.substr(0, message_size-1);
    message_size--;

    //calculate message size
	char tmp[5];
	sprintf(tmp,"%04d", message_size);//write message size 


    //reserver extra space to buffer
    buff.reserve(7+message_size+buff.length());
    //add to buffer
    buff+=tmp;
    buff+=type;
    buff+=message;
}

void codec::append_to_data_buffer(std::string & buff, const std::string & type, std::atomic<double> const * const data, size_t data_length){
    std::string message;
    
    message.reserve((21 * data_length));
    for(size_t i = 0; i<data_length; i++){
        char tmp[21];
        sprintf(tmp, "%F", data[i].load(std::memory_order_relaxed));
        message += tmp;
        message += ",";
    }
    //remove last comma and compute size
    int message_size = message.length();
    message = message.substr(0, message_size-1);
    message_size--;

    //calculate message size
	char tmp[5];
	sprintf(tmp,"%04d", message_size);//write message size 


    //reserver extra space to buffer
    buff.reserve(7+message_size+buff.length());
    //add to buffer
    buff+=tmp;
    buff+=type;
    buff+=message;
}

void codec::create_message(sensor * sen, std::string & buff){
    //clear string of any data
    buff.clear();
    //always add these
    codec::append_to_data_buffer(buff,"XPO",sen->get_atomic_x(),1);
    codec::append_to_data_buffer(buff,"ACC",sen->get_atomic_a(),3);
    codec::append_to_data_buffer(buff,"BRK",sen->get_atomic_brake() ,1);

    codec::append_to_data_buffer(buff,"VEL",sen->get_atomic_v(),3);
    codec::append_to_data_buffer(buff,"HOF",sen->get_atomic_z(),1);
    codec::append_to_data_buffer(buff,"LVH",sen->get_atomic_lev(),2);
    codec::append_to_data_buffer(buff,"TMP",sen->get_atomic_temps(), 8);

    //for now, lets just send everything. We will see how that works
    /*switch(tick){
        case 3:
            codec::append_to_data_buffer(buff,"VEL",get_v(),3);
            codec::append_to_data_buffer(buff,"HOF",get_z(),1);
            break;
        case 1:
            codec::append_to_data_buffer(buff,"LVH",get_lev(),2);
            codec::append_to_data_buffer(buff,"ESC",get_esc(),4);
            codec::append_to_data_buffer(buff,"ATT",get_att(),3);
            break;
        case 2:
            codec::append_to_data_buffer(buff,"VEL",get_v(),3);
            codec::append_to_data_buffer(buff,"TOT",get_tot(),4);
            break;
        default:
            break;
    }
    */

}