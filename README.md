# Warren

**Warren** Workstation access system for GPU job scheduling with Wake on Lan. 

```
 Laptop             Raspberry Pi (always on)         Workstation (asleep)
 thump  --TCP-->    hutch  --Wake-on-LAN-->          network card
        Tailscale                    LAN
```

- **Hutch** is the coordinator daemon on the Pi.
- **Thump** is the CLI. It reaches Hutch over [Tailscale](https://tailscale.com).
- **Worker** runs on the workstation, execute jobs, and suspend the machine when the queue is empty.

## Build

Requires Linux, CMake 3.20, and a C++17 compiler.

```sh
cmake -S . -B build
cmake --build build
```

## Run

On the Pi, put the MAC address of the workstation's wired network card in `.env`. Wake-on-LAN must be enabled in the workstation's BIOS and on that card.

```sh
echo 'WOL_ADDR=aa:bb:cc:dd:ee:ff' > .env
sh deploy/install.sh
sudo journalctl -u hutch -f
```

On the laptop:

```sh
cmake --install build --prefix ~/.local
export HUTCH_ADDR=100.x.y.z  # the Pi's Tailscale address
thump wake
```

Hutch listens on port 8080 on all interfaces and has no authentication yet, so it belongs on a trusted network.
