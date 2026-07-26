#include "poc_basic_text.hpp"
#include "poc_cpu_times_bar_plot.hpp"

int main(void)
{
  try
  {
    // poc_basic_text();
    poc_cpu_times_bar_plot();
  }
  catch (const std::exception &e)
  {
    LOG_ERROR("encountered exception in main loop: ", e.what());

    // temp for development
    std::cerr << std::format("encountered exception in main loop: {}", e.what());
  }
}
