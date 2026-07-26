#include "poc_basic_text.hpp"
#include "poc_tui.hpp"

int main(void)
{
  try
  {
    // poc_basic_text();
    poc_tui();
  }
  catch (const std::exception &e)
  {
    LOG_ERROR("encountered exception in main loop: ", e.what());

    // temp for development
    std::cerr << std::format("encountered exception in main loop: {}", e.what());
  }
}
