#include "poc_basic_text.hpp"
#include "poc_tui.hpp"

#include "logger.hpp"

int main(void)
{
  LOG_INFO("");

  try
  {
    // poc_basic_text();
    PocTui poc_tui;
    poc_tui.run();
  }
  catch (const std::exception &e)
  {
    LOG_ERROR("encountered exception in main loop: ", e.what());

    // temp for development
    std::cerr << std::format("encountered exception in main loop: {}", e.what());
  }
}
