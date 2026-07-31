#include "poc_basic_text.hpp"
#include "poc_tui.hpp"

int main(void)
{
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
