#include <iostream>
#include <stdexcept>
#include "rclcpp/rclcpp.hpp"
#include "rclcpp/serialization.hpp"
#include "rclcpp/serialized_message.hpp"
#include "book_interfaces/msg/readout.hpp"

int main(int argc, char ** argv)
{
  try {
    rclcpp::init(argc, argv);
    using Message = book_interfaces::msg::Readout;
    Message input;
    input.sample_id = 7;
    input.label = "sensor";
    input.payload = {1, 2, 3};
    const auto * support =
      rosidl_typesupport_cpp::get_message_type_support_handle<Message>();
    if (!support) {
      throw std::runtime_error("Missing message type support");
    }
    std::cout << "is_message=" << rosidl_generator_traits::is_message<Message>::value
              << " fixed_size=" << rosidl_generator_traits::has_fixed_size<Message>::value
              << " bounded_size=" << rosidl_generator_traits::has_bounded_size<Message>::value
              << "\ntypesupport=" << support->typesupport_identifier << std::endl;
    rclcpp::Serialization<Message> serializer;
    rclcpp::SerializedMessage bytes;
    serializer.serialize_message(&input, &bytes);
    Message output;
    serializer.deserialize_message(&bytes, &output);
    const bool equal = input == output;
    std::cout << "serialized_bytes=" << bytes.size()
              << "\nsample_id=" << output.sample_id
              << " label=" << output.label
              << " payload_size=" << output.payload.size()
              << "\nroundtrip_equal=" << equal << std::endl;
    rclcpp::shutdown();
    return equal ? 0 : 1;
  } catch (const std::exception & e) {
    std::cerr << "error=" << e.what() << std::endl;
    if (rclcpp::ok()) {
      rclcpp::shutdown();
    }
    return 2;
  }
}
