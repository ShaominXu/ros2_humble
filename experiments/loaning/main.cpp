#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "std_msgs/msg/u_int32.hpp"

template<typename MessageT, typename Fill>
void probe(const rclcpp::Node::SharedPtr & node, const char * topic, Fill fill)
{
  auto pub = node->create_publisher<MessageT>(topic, 10);
  std::cout << "topic=" << topic << " can_loan=" << pub->can_loan_messages() << std::endl;
  {
    auto unpublished = pub->borrow_loaned_message();
    std::cout << "unpublished_valid=" << unpublished.is_valid() << std::endl;
    fill(unpublished.get());
  }
  std::cout << "unpublished_scope_released=1" << std::endl;
  auto message = pub->borrow_loaned_message();
  fill(message.get());
  std::cout << "before_publish_valid=" << message.is_valid() << std::endl;
  pub->publish(std::move(message));
  std::cout << "publish_returned=1 after_publish_valid=" << message.is_valid() << std::endl;
}

int main(int argc, char ** argv)
{
  const std::string mode = argc > 1 ? argv[1] : "plain";
  if (mode != "plain" && mode != "string") { return 2; }
  try {
    rclcpp::init(argc, argv);
    {
      rclcpp::NodeOptions options;
      options.use_intra_process_comms(false);
      options.enable_rosout(false).start_parameter_services(false)
        .start_parameter_event_publisher(false).use_clock_thread(false);
      auto node = std::make_shared<rclcpp::Node>("loaning_probe", options);
      if (mode == "plain") {
        probe<std_msgs::msg::UInt32>(node,"loan_plain",[](std_msgs::msg::UInt32 & m) { m.data=7; });
      } else {
        probe<std_msgs::msg::String>(node,"loan_string",[](std_msgs::msg::String & m) { m.data="sample"; });
      }
    }
    rclcpp::shutdown();
    return 0;
  } catch (const std::exception & e) {
    std::cerr << "error=" << e.what() << std::endl;
    if (rclcpp::ok()) { rclcpp::shutdown(); }
    return 2;
  }
}
