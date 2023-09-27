#include "asio_net/serial_port.hpp"

#include "log.h"

using namespace asio_net;

/**
 * socat -d -d PTY PTY
 */
int main(int argc, char** argv) {
  asio::io_context context;
  std::string device = "/dev/ttys01";
  if (argc >= 2) {
    device = argv[1];
  }
  serial_port serial(context);
  serial.set_reconnect(1000);
  serial.on_try_open = [&] {
    LOG("serial on_try_open: %s", serial.config().device.c_str());
  };
  serial.on_open = [&] {
    LOG("serial on_open:");
    /// set_option
    serial.set_option(asio::serial_port::baud_rate(115200));
    serial.set_option(asio::serial_port::flow_control(asio::serial_port::flow_control::none));
    serial.set_option(asio::serial_port::parity(asio::serial_port::parity::none));
    serial.set_option(asio::serial_port::stop_bits(asio::serial_port::stop_bits::one));
    serial.set_option(asio::serial_port::character_size(asio::serial_port::character_size(8)));

    /// test
    serial.send("hello world");
  };
  serial.on_data = [](const std::string& data) {
    LOG("serial on_data: %s", data.c_str());
  };
  serial.on_open_failed = [](std::error_code ec) {
    LOG("serial on_open_failed: %d, %s", ec.value(), ec.message().c_str());
  };
  serial.on_close = [] {
    LOG("serial on_close:");
  };
  serial.open(device);
  serial.run();
  return 0;
}
