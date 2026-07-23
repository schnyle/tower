# Stress

`stress-ng` is useful for stressing the system.

memory pressure: `stress-ng --vm 1 --vm-bytes 2G --timeout 60s`

cpu pressure: `stress-ng --cpu <N> --timeout 60s --cpu-load 50`
