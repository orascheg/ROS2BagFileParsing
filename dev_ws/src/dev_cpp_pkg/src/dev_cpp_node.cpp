// C/C++
#include <cstdio>

// STD
#include <iostream>
#include <fstream>
#include <ctime>

// ROS
#include "rosbag2_cpp/readers/sequential_reader.hpp"
#include "rosbag2_cpp/typesupport_helpers.hpp"
#include "rosbag2_cpp/converter_interfaces/serialization_format_converter.hpp"
#include "rosbag2_cpp/types/introspection_message.hpp"

// ROS MSG
#include "dev_cpp_pkg/msg/gps_rx.hpp"

using rosbag2_cpp::converter_interfaces::SerializationFormatConverter;

int main(int argc, char ** argv)
{
  (void) argc;
  (void) argv;
  std::ofstream csvFile;
  
  if(argc<2)
  {
    std::cout << "Wrong number of arguments. Usage: " << argv[0] << " <ROS2 bag directory> <csv output file name>" << std::endl;
    return -1;
  }

  csvFile.open(argv[2], std::ofstream::out|std::ofstream::trunc);
  if(!csvFile.is_open())
  {
    std::cout << "Error opening output csv file: " << argv[2];
    return -2;
  }

  rosbag2_cpp::readers::SequentialReader reader;
  rosbag2_cpp::StorageOptions storage_options{};
  
  storage_options.uri = std::string(argv[1]);
  storage_options.storage_id = "sqlite3";

  rosbag2_cpp::ConverterOptions converter_options{};
  converter_options.input_serialization_format = "cdr";
  converter_options.output_serialization_format = "cdr";
  reader.open(storage_options, converter_options);
  auto topics = reader.get_all_topics_and_types();
  for (auto t:topics)
  {
    std::cout << "meta name: " << t.name << std::endl;
    std::cout << "meta type: " << t.type << std::endl;
    std::cout << "meta serialization_format: " << t.serialization_format << std::endl;
  }
  
  // read and deserialize "serialized data"
  bool firstMsg=true;
  dev_cpp_pkg::msg::GpsRx msg;
  auto library = rosbag2_cpp::get_typesupport_library("dev_cpp_pkg/msg/GpsRx", "rosidl_typesupport_cpp");
  auto type_support = rosbag2_cpp::get_typesupport_handle("dev_cpp_pkg/msg/GpsRx", "rosidl_typesupport_cpp", library);
  auto ros_message = std::make_shared<rosbag2_cpp::rosbag2_introspection_message_t>();
  rosbag2_cpp::SerializationFormatConverterFactory factory;
  std::unique_ptr<rosbag2_cpp::converter_interfaces::SerializationFormatDeserializer> cdr_deserializer_;

  while(reader.has_next())
  {
    // serialized data
    auto serialized_message = reader.read_next();
    
    // set up things at the first entry
    if(firstMsg)
    {
      // write header
      csvFile << "DateTime,Latitude,Longitude,Height"<< std::endl;
      // initialize structures
      ros_message->time_stamp = 0;
      ros_message->message = nullptr;
      ros_message->allocator = rcutils_get_default_allocator();
      ros_message->message = &msg;
      cdr_deserializer_ = factory.load_deserializer("cdr");
      firstMsg=false;
    }

    // deserialization and conversion to ros message
    cdr_deserializer_->deserialize(serialized_message, type_support, ros_message);

    // ros message data
    std::cout << std::endl;
    std::cout << msg.gps_latitude << std::endl;
    std::cout << msg.gps_longitude << std::endl;
    std::cout << msg.gps_height << std::endl;
    std::cout << msg.solution_status << std::endl;

    // convert the timestamps to human readable format
    std::time_t msgTime = time_t((long int)(ros_message->time_stamp/1000000000));
    int nanoseconds = ros_message->time_stamp % 1000000000;
    char dateString[256];
    std::strftime(dateString, sizeof(dateString), "%F %T", std::localtime(&msgTime));

    // write the actual message to the output file
    csvFile << dateString << "." << nanoseconds << "," << msg.gps_latitude << "," <<  msg.gps_longitude << "," << msg.gps_height << std::endl;
  }
  csvFile.close();
  return 0;
}