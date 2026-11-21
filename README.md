# Warren

**Warren** is a self-hosted GPU job scheduler.

- **Hutch** is the coordinator. It runs on the Raspberry Pi 5, queues GPU jobs from anywhere, wakes the workstation to run them, and puts it back to sleep when the queue is empty.
- **Thump** is the CLI. You use it to submit jobs from the laptop or phone over [Tailscale](https://tailscale.com).
