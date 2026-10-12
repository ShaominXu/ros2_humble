#include <iostream>
#include <memory>
#include <string>
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

void pause_at(const char * stage)
{
  std::cout << stage << " [press Enter]" << std::endl;
  std::string line;
  std::getline(std::cin, line);
}

int main(int argc, char ** argv)
{
  const bool close_first = argc > 1 && std::string(argv[1]) == "shutdown";
  auto context = std::make_shared<rclcpp::Context>();
  context->init(0, nullptr);
  std::weak_ptr<rclcpp::Context> weak_context = context;
  {
    rclcpp::NodeOptions options;
    options.context(context);
    options.start_parameter_services(false);
    options.start_parameter_event_publisher(false);
    options.enable_rosout(false);
    auto node = std::make_shared<rclcpp::Node>("lifetime_probe", options);
    std::weak_ptr<rclcpp::Node> weak_node = node;
    std::weak_ptr<rclcpp::node_interfaces::NodeBaseInterface> weak_base =
      node->get_node_base_interface();
    auto publisher = node->create_publisher<std_msgs::msg::String>("probe", 10);
    pause_at("A: objects created");
    if (close_first) {
      context->shutdown("lifetime experiment");
      std::cout << "context_valid=" << context->is_valid()
                << " node_alive=" << !weak_node.expired()
                << " publisher_alive=" << static_cast<bool>(publisher) << std::endl;
      pause_at("B: context shutdown; objects retained");
    }
    node.reset();
    std::cout << "node_expired=" << weak_node.expired()
              << " base_expired=" << weak_base.expired() << std::endl;
    pause_at("C: outer Node released; publisher retained");
    publisher.reset();
    std::cout << "base_expired=" << weak_base.expired() << std::endl;
    pause_at("D: publisher released");
  }  // NodeOptions also holds Context; release it before the final observation.
  if (context->is_valid()) {
    context->shutdown("lifetime experiment completed");
  }
  context.reset();
  std::cout << "context_expired=" << weak_context.expired() << std::endl;
  return 0;
}
