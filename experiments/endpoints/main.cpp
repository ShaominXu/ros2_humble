#include <chrono>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

int run_probe(const std::string & mode)
{
  using namespace std::chrono_literals;
  using Message = std_msgs::msg::String;
  rclcpp::NodeOptions options;
  options.use_intra_process_comms(false);
  options.enable_rosout(false);
  options.start_parameter_services(false);
  options.start_parameter_event_publisher(false);
  options.use_clock_thread(false);
  auto node = std::make_shared<rclcpp::Node>(
    mode == "publish" ? "endpoint_pub" : "endpoint_sub", options);
  if (mode == "publish") {
    auto qos = rclcpp::QoS(rclcpp::KeepLast(7)).reliable().durability_volatile();
    auto pub = node->create_publisher<Message>("stream", qos);
    std::cout << "topic=" << pub->get_topic_name()
              << " depth=" << pub->get_actual_qos().get_rmw_qos_profile().depth << std::endl;
    const auto deadline = std::chrono::steady_clock::now() + 10s;
    while (rclcpp::ok() && pub->get_subscription_count() == 0 &&
      std::chrono::steady_clock::now() < deadline)
    {
      std::this_thread::sleep_for(50ms);
    }
    if (!rclcpp::ok() || pub->get_subscription_count() == 0) {
      std::cerr << "No subscription observed within 10 seconds" << std::endl;
      return 1;
    }
    for (int i = 1; i <= 5 && rclcpp::ok(); ++i) {
      Message msg;
      msg.data = "sample:" + std::to_string(i);
      pub->publish(msg);
      std::cout << "publish_returned=" << msg.data << std::endl;
      std::this_thread::sleep_for(200ms);
    }
    std::this_thread::sleep_for(1s);
    return 0;
  }
  int callback_count = 0;
  int take_count = 0;
  auto qos = rclcpp::QoS(rclcpp::KeepLast(10)).reliable().durability_volatile();
  auto sub = node->create_subscription<Message>(
    "stream", qos, [&callback_count](Message::ConstSharedPtr msg) {
      ++callback_count;
      std::cout << "callback=" << msg->data << std::endl;
    });
  std::cout << "topic=" << sub->get_topic_name()
            << " depth=" << sub->get_actual_qos().get_rmw_qos_profile().depth << std::endl;
  const auto deadline = std::chrono::steady_clock::now() + 30s;
  while (rclcpp::ok() && std::chrono::steady_clock::now() < deadline) {
    if (mode == "callback") {
      rclcpp::spin_some(node);
      if (callback_count >= 5) { break; }
    } else {
      Message msg;
      rclcpp::MessageInfo info;
      if (sub->take(msg, info)) {
        ++take_count;
        std::cout << "take=" << msg.data
                  << " from_intra_process="
                  << info.get_rmw_message_info().from_intra_process << std::endl;
      }
      if (take_count >= 5) { break; }
    }
    std::this_thread::sleep_for(20ms);
  }
  std::cout << "callback_count=" << callback_count << " take_count=" << take_count << std::endl;
  return callback_count == 5 || take_count == 5 ? 0 : 1;
}

int main(int argc, char ** argv)
{
  const std::string mode = argc > 1 ? argv[1] : "";
  if (mode != "publish" && mode != "callback" && mode != "take") {
    std::cerr << "Usage: endpoint_probe publish|callback|take [ROS arguments]" << std::endl;
    return 2;
  }
  try {
    rclcpp::init(argc, argv);
    int result = run_probe(mode);
    rclcpp::shutdown();
    return result;
  } catch (const std::exception & e) {
    std::cerr << "error=" << e.what() << std::endl;
    if (rclcpp::ok()) { rclcpp::shutdown(); }
    return 2;
  }
}
