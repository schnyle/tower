# Tower Design Document

## Features

- System metrics (CPU, memory, network, disk)
- Process monitoring
- Available package updates
- systemd journal entries (?)
- Atlas health/status

## Architecture

- TUI with `termios`
- Metrics polling from `/proc` and `/sys`
