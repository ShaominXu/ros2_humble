#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

int main(int argc, char ** argv)
{
  using namespace std::chrono_literals;
  if (argc < 4 || (std::string(argv[1]) != "pub" && std::string(argv[1]) != "sub") ||
    (std::string(argv[2]) != "reliable" && std::string(argv[2]) != "best-effort") ||
    (std::string(argv[3]) != "volatile" && std::string(argv[3]) != "transient-local"))
  {
    std::cerr << "Usage: qos_probe pub|sub reliable|best-effort volatile|transient-local" << std::endl;
    return 2;
  }
  const bool publisher = std::string(argv[1]) == "pub";
  try {
    rclcpp::init(argc, argv);
    {
      rclcpp::NodeOptions options;
      options.use_intra_process_comms(false);
      options.enable_rosout(false).start_parameter_services(false)
        .start_parameter_event_publisher(false).use_clock_thread(false);
      auto node = std::make_shared<rclcpp::Node>(publisher ? "qos_pub" : "qos_sub", options);
      rclcpp::QoS qos(rclcpp::KeepLast(5));
      if (std::string(argv[2]) == "reliable") { qos.reliable(); } else { qos.best_effort(); }
      if (std::string(argv[3]) == "volatile") { qos.durability_volatile(); }
      else { qos.transient_local(); }
      const auto start = std::chrono::steady_clock::now();
      if (publisher) {
        auto pub = node->create_publisher<std_msgs::msg::String>("qos_probe", qos);
        int sequence = 0;
        std::cout << "topic=" << pub->get_topic_name() << " phase=publishing" << std::endl;
        while (rclcpp::ok() && std::chrono::steady_clock::now() - start < 10s) {
          std_msgs::msg::String msg;
          msg.data = "sample:" + std::to_string(++sequence);
          pub->publish(msg);
          std::cout << "publish_returned=" << msg.data << std::endl;
          std::this_thread::sleep_for(200ms);
        }
        std::cout << "phase=holding_no_new_messages" << std::endl;
        while (rclcpp::ok() && std::chrono::steady_clock::now() - start < 25s) {
          std::this_thread::sleep_for(100ms);
        }
      } else {
        int received = 0;
        auto sub = node->create_subscription<std_msgs::msg::String>("qos_probe", qos,
          [&received](std_msgs::msg::String::ConstSharedPtr msg) {
            ++received;
            std::cout << "received=" << msg->data << std::endl;
          });
        std::cout << "topic=" << sub->get_topic_name() << std::endl;
        while (rclcpp::ok() && std::chrono::steady_clock::now() - start < 10s) {
          rclcpp::spin_some(node);
          std::this_thread::sleep_for(20ms);
        }
        std::cout << "received_count=" << received << std::endl;
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
