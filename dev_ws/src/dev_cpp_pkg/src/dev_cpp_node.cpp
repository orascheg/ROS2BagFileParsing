// C/C++
#include <cstdio>

// STD
#include <iostream>
#include <fstream>
#include <ctime>
#include <iomanip> 

// ROS
#include "rclcpp/rclcpp.hpp"
#include "rosbag2_cpp/readers/sequential_reader.hpp"
#include "rosbag2_cpp/typesupport_helpers.hpp"
#include "rosbag2_cpp/converter_interfaces/serialization_format_converter.hpp"
#include "rosbag2_cpp/types/introspection_message.hpp"

// ROS MSG
#include "dev_cpp_pkg/msg/gps_rx.hpp"
#include "sensor_msgs/msg/nav_sat_fix.hpp"
#include "rcl_interfaces/msg/log.hpp"

using rosbag2_cpp::converter_interfaces::SerializationFormatConverter;

// convert the timestamps to human readable format
// return a datesting
std::string convertNanosecondsSinceEpoc(long unsigned int nanoSinceEpoc)
{
  std::time_t msgTime = time_t((long int)(nanoSinceEpoc/1000000000));
  int nanoseconds = nanoSinceEpoc % 1000000000;
  char dateString[256];
  std::strftime(dateString, sizeof(dateString), "%F %T", std::localtime(&msgTime));
  return std::string(dateString)+"."+std::to_string(nanoseconds); 
}


int main(int argc, char ** argv)
{
  (void) argc;
  (void) argv;
  std::ofstream csvFile;
  std::ofstream logMessages;
  rosbag2_storage::StorageFilter storage_filter;

  if(argc<3)
  {
    std::cout << "Wrong number of arguments. Usage: " << argv[0] << " <ROS2bag directory> <gps csv output file name> <log messages file name>" << std::endl;
    return -1;
  }

  // open the gps data log file
  csvFile.open(argv[2], std::ofstream::out|std::ofstream::trunc);
  if(!csvFile.is_open())
  {
    std::cout << "Error opening output csv file: " << argv[2];
    return -2;
  }
  csvFile << std::fixed << std::setprecision(10) << std::endl;

  // open the file for the message log (empty if /rosout was not recorded)
  logMessages.open(argv[3], std::ofstream::out|std::ofstream::trunc);
  if(!logMessages.is_open())
  {
    std::cout << "Error opening output log file: " << argv[3];
    return -5;
  }

  rosbag2_cpp::readers::SequentialReader reader;
  rosbag2_cpp::StorageOptions storage_options{};
  
  storage_options.uri = std::string(argv[1]);
  storage_options.storage_id = "sqlite3";

  rosbag2_cpp::ConverterOptions converter_options{};
  converter_options.input_serialization_format = "cdr";
  converter_options.output_serialization_format = "cdr";
  reader.open(storage_options, converter_options);
  std::vector<rosbag2_storage::TopicMetadata> topics = reader.get_all_topics_and_types();
  std::map<std::string,std::string> nameTypeMap;
  for (auto t:topics)
  {
    std::cout << "meta name: " << t.name << std::endl;
    std::cout << "meta type: " << t.type << std::endl;
    std::cout << "meta serialization_format: " << t.serialization_format << std::endl;
    nameTypeMap[t.name]=t.type;
  }

  // create the type support for the supported message types
  // todo: library should be able to support more than one type?
  auto library_log = rosbag2_cpp::get_typesupport_library("rcl_interfaces/msg/Log", "rosidl_typesupport_cpp");
  auto type_support_log = rosbag2_cpp::get_typesupport_handle("rcl_interfaces/msg/Log", "rosidl_typesupport_cpp", library_log);
  auto library_gps = rosbag2_cpp::get_typesupport_library("sensor_msgs/msg/NavSatFix", "rosidl_typesupport_cpp");
  auto type_support_gps = rosbag2_cpp::get_typesupport_handle("sensor_msgs/msg/NavSatFix", "rosidl_typesupport_cpp", library_gps);
  auto library_gpx = rosbag2_cpp::get_typesupport_library("dev_cpp_pkg/msg/GpsRx", "rosidl_typesupport_cpp");
  auto type_support_gpx = rosbag2_cpp::get_typesupport_handle("dev_cpp_pkg/msg/GpsRx", "rosidl_typesupport_cpp", library_gpx);
  auto ros_message = std::make_shared<rosbag2_cpp::rosbag2_introspection_message_t>();
  rosbag2_cpp::SerializationFormatConverterFactory factory;
  // todo: check if the deserialization format is really cdr
    std::unique_ptr<rosbag2_cpp::converter_interfaces::SerializationFormatDeserializer> cdr_deserializer;
  cdr_deserializer = factory.load_deserializer("cdr");
  std::shared_ptr<rosbag2_storage::SerializedBagMessage> serialized_message;
  ros_message->allocator = rcutils_get_default_allocator();

  // prepare the csv file for gps
  csvFile << "DateTime,Latitude,Longitude,Height"<< std::endl;
  while(reader.has_next())
  {
    // read serialized data into the message
    serialized_message = reader.read_next();

    //std::cout << "I read data for " << serialized_message->topic_name << " of length: " << serialized_message->serialized_data->buffer_length <<" with type: "<< nameTypeMap[serialized_message->topic_name]<< std::endl;
    // Print the serialized data message in HEX representation
    // This output corresponds to what you would see in e.g. Wireshark
    // when tracing the RTPS packets.
    /*for (size_t i = 0; i < serialized_message->serialized_data->buffer_length; ++i)
    {
      printf("%02x ", serialized_message->serialized_data->buffer[i]);
    }
    std::cout << std::endl;*/
    ros_message->time_stamp = 0;
    ros_message->message = nullptr;

    if(strcmp(nameTypeMap[serialized_message->topic_name].c_str(),"rcl_interfaces/msg/Log")==0)
    {
      // this is a ROS2 log message, deserialize it with the proper type support
      rcl_interfaces::msg::Log message_log;
      ros_message->message = &message_log;
      cdr_deserializer->deserialize(serialized_message, type_support_log, ros_message);

      std::string output_string=convertNanosecondsSinceEpoc((long unsigned int)ros_message->time_stamp);
      // convert log level
      int logLevel=message_log.level;
      std::string level_out;
      switch(logLevel)
      {
        case 10:
          level_out="Debug";break;
        case 20:
          level_out="Info";break;
        case 30:
          level_out="Warning";break;
        case 40:
          level_out="Error";break;
        case 50:
          level_out="Fatal";break;
        default:
          level_out="Undefinded";
      }

      logMessages << output_string << " [" << level_out << "]:" <<  message_log.msg << " from " << message_log.file << " function " << message_log.function << " line " << message_log.line << std::endl;
    }
    else if(strcmp(nameTypeMap[serialized_message->topic_name].c_str(),"sensor_msgs/msg/NavSatFix")==0)
    {
      // this is a NavSatFix message from gps node
      sensor_msgs::msg::NavSatFix message_gps;
      ros_message->message = &message_gps;
      cdr_deserializer->deserialize(serialized_message, type_support_gps, ros_message);

      std::string output_string=convertNanosecondsSinceEpoc((long unsigned int)ros_message->time_stamp);
      // write the content to the output file
      csvFile << output_string << "," << message_gps.latitude << "," <<  message_gps.longitude << "," << message_gps.altitude << std::endl;
    }
    else if(strcmp(nameTypeMap[serialized_message->topic_name].c_str(),"r2dpac_msgs/msg/GpsRx")==0)
    {
      // gpx message used in the original example
      dev_cpp_pkg::msg::GpsRx message_gpx;
      ros_message->message = &message_gpx;
      cdr_deserializer->deserialize(serialized_message, type_support_gpx, ros_message);

      // write the content to the output file
      std::string output_string=convertNanosecondsSinceEpoc((long unsigned int)ros_message->time_stamp);
      csvFile << output_string << "," << message_gpx.lat << "," <<  message_gpx.lon << "," << message_gpx.height << std::endl;
    }
    else
    {
      // instead of this the supported messages can be extended
      std::cout << "Unknown message type " << nameTypeMap[serialized_message->topic_name] << std::endl;
    }
  }
  csvFile.close();
  return 0;
}