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
