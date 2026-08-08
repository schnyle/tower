# Notes

## Questions

- Numeric type for plotted `RingBuffer`s.
  - Option A (space efficient): store all scraped data as the smallest required type, then convert to floating point when drawing the bar plot
  - Option B (time efficient): store all scraped data as the same 64-bit floating point, then bar plot does not need to (repeatedly) convert this data for computing bar plots
- Circular import with draw/window for `Rect`

## TODO

- Tui/Canvas/FrameBuffer/Window ownership/responsibility
- Config file and runtime configuration
- UI interaction
- Per-process collection/plotting
- System for convenient window placement
- separation of raw values from computed (where does computing derived values happen?)
- terminal size change
- runtime configuration
- revisit logging (use a 3rd party lib?)
- proper TDD for drawing functions

## Design

- `Canvas`: the data structure which defines the visual content rendered on a TUI. Provides methods for safe & convenient updates to the content to be rendered to the TUI.
  - `canvas.hpp` also defines `Cell`, the base unit that represents one character rendered to the screen
  - `canvas.hpp` also defines `Color`, the RGB value used to render `Cell`'s with color.
  - `Cell` is used throughout a `Canvas`. Each `Cell` includes a `Color`. `Color` is not used directly by a `Canvas`.
- `windows/`: class hierarchy responsible for owning a "window" or "panel" on the TUI.
  - Abstract class owns the window name and its position `Rect`. Provides a `void draw(Canvas &)` function to update the canvas.
  - Subclasses take references to data to be displayed in the content of the window. They maintain other data relevant to that window type (`BarPlotWindow::ymin/ymax` + a function for formatting the title string)
  - `drawing/` provide `void draw_something(Canvas&, Rect, Data &)` helpers for the nitty-gritty content rendering logic
- `parsers/`: defines `Parser`s which query the system for data through the file system.
  - A `Parser`, defined in `parser.hpp`, provides a `Data` member, a `struct` that defines the data the parser produces, and `std::optional<Data> parse(std::istream&)`, which contains the logic for converting the file input stream to a `Data` struct.
  - Parsers will grow to be able to handle a large amount of the data available, including files under `/proc`, `/sys`, and `/etc`.
  - Open question: let's say a file contains 30 metrics, but the configuration only cares about 3 for display. Do we still parse all 30, but only store the 3 for long term? Additionally, how do we want to handle error reads? If 1 of 30 hits an error, do we throw out the whole batch?
- `include/`: contains scattered header files which have yet to find a home
  - `FrameBuffer`: owns a pair of front/back Canvas's. Provides public access to the back Canvas for windows/drawers to update. Handles double-buffered rendering of text to the screen.
  - `RingBuffer`: core data structure for keeping time series data points.
