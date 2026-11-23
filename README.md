# Warren

**Warren** is a self-hosted GPU job scheduler.

- **Hutch** is the coordinator. It runs on a Raspberry Pi, queues GPU jobs from anywhere, wakes the workstation to run them, and puts it back to sleep when the queue is empty.
- **Thump** is the CLI. You use it to submit jobs from the laptop or phone over [Tailscale](https://tailscale.com).

## Build and run

```sh
cmake -S . -B build
cmake --build build
WOL_ADDR=aa:bb:cc:dd:ee:ff ./build/hutch/hutch
```

`WOL_ADDR` is the MAC address of the workstation's wired network card. The workstation needs Wake-on-LAN enabled in its BIOS and on that card.
