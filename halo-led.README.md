# AI-HALO-LED-Control

Command-line control for the multicolour status LED on the
[AMD Ryzen™ AI Halo Developer Platform](https://www.amd.com/en/products/processors/desktops/ryzen/ryzen-ai-halo/ryzen-ai-max-plus-395.html)
— the Ryzen™ AI Max+ 395 desktop platform, board ID `RAH-001`.

The LED is exposed by the platform's WMI driver as a standard Linux multicolour
LED, so it can be driven straight from sysfs. `halo-led` wraps that with named
colours, sane validation, and a workaround for a driver bug that corrupts the
LED's state (see [The driver bug](#the-driver-bug-this-works-around)).

This LED is **not** a hardware fault indicator. It is a user-controllable RGB
light with its kernel trigger set to `none`, meaning nothing in the kernel or
firmware drives it — its colour is whatever the last userspace write set it to.
A red LED here means "someone picked red", not "something is broken".

- **Script:** `halo-led`
- **Device:** `/sys/class/leds/amd_halo:multicolor:status`
- **Driver:** `amd_halo_led` (WMI GUID `081E747B-E028-4232-AF24-EAAEAB2B1E86`),
  on top of `led_class_multicolor`
- **Verified on:** AMD Ryzen™ AI Max+ 395 (`RAH-001`), kernel
  `6.18.35+rex+2-amd64`, BIOS `03.03` (2026-06-10)

Everything documented here is measured behaviour on that configuration. The
driver bug in particular is worth re-testing after a kernel or BIOS update.

---

## Requirements

> **This currently only works on AMD's own Halo OS kernel.** The `amd_halo_led`
> driver is **not in mainline Linux** as of July 2026, so on a stock
> Debian/Ubuntu/Fedora kernel the LED device does not exist and this script
> cannot do anything. Read [Compatibility](#compatibility) before installing.

- **Hardware:** AMD Ryzen™ AI Halo Developer Platform (Ryzen™ AI Max+ 395,
  `RAH-001`).
- **Kernel:** one providing the `amd_halo_led` module, such that
  `/sys/class/leds/amd_halo:multicolor:status` exists. Check with:
  ```sh
  lsmod | grep amd_halo_led
  ls -d /sys/class/leds/amd_halo:*
  ```
- **Tools:** `bash` and `curl`.
- **Root:** not required on Halo OS. Required everywhere else — see
  [Compatibility](#compatibility) and [How it writes](#how-it-writes).

## Compatibility

Read this before reinstalling your box, or before porting this elsewhere. Three
things this script leans on are shipped by AMD's Halo OS, not by Linux in
general.

### 1. The driver is not upstream yet

`amd_halo_led` is **not in mainline Linux**. As of its
[initial posting](https://www.phoronix.com/news/AMD-Halo-Box-RGB-LED-Driver)
(28 April 2026) it was still under review on the `platform-driver-x86` mailing
list, speculatively targeted at around Linux v7.2. AMD ships it in their own
kernel — `6.18.35+rex+2-amd64` on this machine.

So on a stock distro kernel there is **no LED device at all**. Nothing to
control, and no amount of scripting fixes that. You would need to wait for the
driver to reach mainline, or build/backport it yourself.

Be aware that `modinfo amd_halo_led` reports `intree: Y`, which is misleading —
it only means the module was built inside the kernel tree it shipped with
(AMD's), **not** that it is in mainline.

### 2. The LED device name may change

The upstream driver as posted creates:

```
/sys/class/leds/amd_halo:rgb:light_bar/
```

while the driver shipped on this platform creates:

```
/sys/class/leds/amd_halo:multicolor:status/
```

This script hardcodes the latter, in `LED_DIR` at the top of the file. If AMD
converges on the upstream naming, the path changes and the script breaks —
**including on this box, via a routine kernel update**.

If `halo-led` suddenly cannot find the device, see what is actually present and
update `LED_DIR` to match:

```sh
ls -d /sys/class/leds/amd_halo:*
```

### 3. The no-root path is Halo OS only

Two Halo-OS-specific pieces are what make `sudo` unnecessary here:

- **`halo-lp.service`** — the Halo Center web app on port 10000, which provides
  the REST API fallback. Ships in AMD's `halo-lp` package.
- **`/usr/lib/tmpfiles.d/amd-halo-box.conf`** — from the `amd-halo-box` package,
  which grants the `halo-lp` group write access to the LED attributes:

  ```
  z /sys/class/leds/amd_halo:multicolor:status/brightness      0664 root halo-lp - -
  z /sys/class/leds/amd_halo:multicolor:status/multi_intensity 0664 root halo-lp - -
  ```

  (Note: AMD's own backend source comments attribute these permissions to udev
  rules. That is incorrect — there is no udev rule for this LED; the permissions
  come from systemd-tmpfiles.)

On any other distro neither exists, so the LED attributes stay root-owned and
you must run the script with `sudo`.

### Summary

| Setup | Works? |
|---|---|
| Halo OS (AMD's shipped image) | Yes — no root needed |
| Other distro, stock kernel | **No** — driver absent, device does not exist |
| Other distro + driver built/backported | Yes, but needs `sudo`, and likely a `LED_DIR` change |
| Future kernel carrying the upstream driver | Likely needs a `LED_DIR` change (see above) |

What *is* portable: the script logic (plain `bash` + `curl`), the 0–100 colour
semantics, and the `brightness=0` workaround — that bug tracks the driver, not
the distro.

## Install

```sh
git clone https://github.com/brainchillz/AI-HALO-LED-Control.git
cd AI-HALO-LED-Control
chmod +x halo-led
./halo-led status
```

Optionally put it on your `PATH`, so the bare `halo-led` examples below work as
written:

```sh
mkdir -p ~/.local/bin && ln -s "$PWD/halo-led" ~/.local/bin/halo-led
```

(`~/.local/bin` is already on `PATH` in Debian's default `.profile`; log out and
back in, or `source ~/.profile`, to pick it up.)

## Usage

```
halo-led status              # show current colour + brightness
halo-led off                 # off, remembering the colour
halo-led on                  # back on with the remembered colour
halo-led <colour> [bright]   # named colour, optional brightness 1-100
halo-led rgb <R> <G> <B> [bright]
halo-led persist             # reapply the current colour at every boot (root)
halo-led unpersist           # stop reapplying at boot (root)
```

Examples:

```sh
halo-led status
halo-led green               # green, full brightness
halo-led blue 40             # blue, dimmed to 40
halo-led rgb 100 40 0        # custom orange
halo-led rgb 0 100 0 25      # dim green
halo-led off
halo-led on
```

Runs as an ordinary user — no `sudo` needed, except `persist` / `unpersist`.
See [How it writes](#how-it-writes).

---

## Surviving a reboot

**Set a colour and it is gone at the next boot.** The LED's colour lives in
kernel memory, which does not survive power-off, and the platform firmware
reasserts its own default early in boot. Nothing on Halo OS reapplies your
colour — `halo-lp.service` only writes the LED when the dashboard or the API
asks it to, and the tmpfiles entries set permissions, not values. So *something*
has to write the colour back on every boot. The only question is what.

The lightest answer is a **udev rule**, which is what `persist` installs:

```sh
halo-led green 80          # get the LED how you want it
sudo halo-led persist      # snapshot that, reapply it at every boot
```

That is it. No unit to install, enable, or maintain — `systemd-udevd` already
runs, and the rule fires on the LED's `add` event, the moment `amd_halo_led`
probes and creates the device. `persist` snapshots whatever the LED shows *right
now*, so to change the boot colour, set the new one and run it again. `off`
persists fine too — an all-zero mix is a legitimate thing to restore.

The rule it writes is a single line, and there is nothing clever in it:

```
# /etc/udev/rules.d/99-halo-led.rules
ACTION=="add", SUBSYSTEM=="leds", KERNEL=="amd_halo:multicolor:status", \
  ATTR{multi_intensity}="0 100 0", ATTR{brightness}="80"
```

Edit it by hand if you prefer; `halo-led unpersist` deletes it. The attribute
order matters — `multi_intensity` is written before `brightness`, and
`brightness` is never `0`, for the reason in
[The driver bug](#the-driver-bug-this-works-around).

After a reboot, confirm with `halo-led status`.

### Why not a systemd service?

A `oneshot` unit would also work, and is what most guides reach for. It is
strictly more machinery for the same result: a unit file, an `enable`, and an
ordering dependency on the LED device actually existing — which udev gives for
free, because the `add` event *is* the device appearing. Reach for a service
only if you need something a rule cannot express, e.g. a colour that depends on
runtime state.

### The driver reflects hardware state at probe

Worth knowing if you test this by reloading the module rather than rebooting:
`amd_halo_led` reads the LED's *emitted* colour back from hardware when it
probes, and reports it with `brightness` normalised to `100`. Reloading the
module with blue at `0 0 100`, brightness `80`, brings the device back as:

```
multi_intensity = 0 0 80     brightness = 100
```

Same light, different split — `0 0 100 @ 80` and `0 0 80 @ 100` are the same
emitted colour (see [The two knobs](#the-two-knobs)). This means the EC keeps
the colour across a *module reload*, so a reload is **not** a faithful test of
what a cold boot does; the firmware default only reappears on a real power
cycle. It does not affect the rule either way — the rule fires on `add`,
whatever state the device comes up in.

---

## Colour value setup

### Values are 0–100, not 0–255

This is the single most common mistake. The LED class exposes a **0–100** scale
on every channel, and `max_brightness` is `100`. Feeding it 8-bit values
silently clamps: `rgb 255 0 0` is rejected by this script, and a raw
`echo "255 0 0"` to sysfs just pins red to its maximum.

To convert an ordinary 8-bit RGB colour, divide each channel by 2.55:

| 8-bit RGB | ÷ 2.55 | `halo-led` command |
|---|---|---|
| `255, 0, 0` | `100, 0, 0` | `halo-led rgb 100 0 0` |
| `255, 102, 0` | `100, 40, 0` | `halo-led rgb 100 40 0` |
| `128, 0, 255` | `50, 0, 100` | `halo-led rgb 50 0 100` |

### The two knobs

The device exposes two sysfs attributes, and they do different jobs:

| Attribute | Range | Meaning |
|---|---|---|
| `multi_intensity` | three ints, `0`–`100` each | Per-channel colour mix |
| `brightness` | `0`–`100` | Master level applied across all channels |

Channel order comes from `multi_index`, which on this hardware reads:

```
red green blue
```

So `multi_intensity` is written as `"R G B"` — e.g. `printf '0 100 0'` is green.

The emitted light is the colour scaled by the master level, roughly:

```
output(channel) = multi_intensity(channel) * brightness / 100
```

That means **colour and brightness are independent**: `rgb 0 100 0 25` and
`rgb 0 25 0 100` both produce a dim green, but the first keeps a saturated green
mix and turns the whole LED down, which is usually what you want. Prefer
adjusting `brightness` for dimming and leave the mix at full saturation.

Two consequences worth internalising:

- An all-zero mix (`0 0 0`) is **off at any brightness** — there is no colour to
  scale. This is how `off` is implemented.
- A non-zero mix at `brightness=1` is on, but very dim. The factory default this
  box shipped with was `1 0 0` — red at intensity 1 of 100, i.e. the dimmest
  possible non-zero red.

### Named colours

| Name | R | G | B |
|---|---|---|---|
| `red` | 100 | 0 | 0 |
| `green` | 0 | 100 | 0 |
| `blue` | 0 | 0 | 100 |
| `yellow` | 100 | 100 | 0 |
| `cyan` | 0 | 100 | 100 |
| `magenta` | 100 | 0 | 100 |
| `white` | 100 | 100 | 100 |
| `orange` | 100 | 40 | 0 |
| `purple` | 50 | 0 | 100 |

---

## The driver bug this works around

**Writing `0` to `brightness` does not turn this LED off. It corrupts the LED's
state.** Two things happen:

1. The value **bounces back to `100`** — you write off, you get full brightness.
2. A moment later the driver **asynchronously zeroes `multi_intensity`**,
   destroying the colour.

The second part is what makes this genuinely nasty. The wipe is *deferred*, so
an immediate read-back looks correct and the bug hides. It only surfaces later,
when something reads the colour and finds `0 0 0`.

### Measured behaviour

Every other brightness value round-trips correctly. Only `0` misbehaves:

```
wrote brightness=100  ->  reads 100   ok
wrote brightness=75   ->  reads  75   ok
wrote brightness=50   ->  reads  50   ok
wrote brightness=0    ->  reads 100   BOUNCED, and colour wiped shortly after
```

The deferred wipe, observed directly:

```
wrote multi_intensity='0 0 100'  ->  intensity='0 0 100'  brightness=60
wrote brightness='0'             ->  intensity='0 0 100'  brightness=100   # colour still looks fine
wrote brightness='50'            ->  intensity='0 0 0'    brightness=50    # colour gone
```

This bit the first version of this script: `off` wrote `brightness=0` and
reported success, then `on` read back the wiped `0 0 0`, concluded the LED had
no colour, and lit it **white** instead of restoring the previous colour.

### The workaround

The script **never writes `brightness=0`**. Instead:

- `off` writes `multi_intensity = "0 0 0"` and leaves `brightness` untouched.
  This is stable and verified — it stays off, with no bounce and no async
  surprise.
- The previous colour is saved to `~/.local/state/halo-led.color`
  (honours `$XDG_STATE_HOME`) so `on` can restore it.
- Passing brightness `0` explicitly is **refused** with a pointer to `off`,
  rather than silently corrupting state.

### This affects the web UI too

The Halo Center dashboard writes the *same* sysfs attributes through the same
endpoint, and its API schema accepts `brightness: 0`
(`Field(..., ge=0, le=100)` in `/usr/lib/halo-lp/backend/main.py`). So a
brightness slider that reaches zero can trip this identically.

If the LED ever jumps to white or full brightness after you dim it in the
dashboard, that is this bug — not failing hardware. Recover with:

```sh
halo-led green      # or any colour; re-establishes a known mix
```

### Things that are *not* problems

Ruled out by measurement, so nobody re-investigates them:

- **No concurrent writer.** The LED does not change on its own. Sampled once a
  second with no writes: perfectly stable.
- **No write race.** Rapid back-to-back writes to `multi_intensity` and
  `brightness` with no settle delay are reliable — five consecutive no-delay
  runs all landed correctly. The script needs no `sleep` between writes.

The *only* anomaly is `brightness=0`.

### Upstream status

This has **not** been reported to the `platform-driver-x86` mailing list.

Whether the fault lives in the driver's brightness handling or in the WMI
firmware underneath it cannot be determined from userspace — everything above is
black-box observation via sysfs. Someone reading the driver source could likely
tell at a glance.

Since the driver is still under review upstream (see
[Compatibility](#compatibility)), this is a good moment to fix it properly
rather than work around it forever. The authors listed in `modinfo` are
Leo Lin (`Leo.Lin@amd.com`) and Mario Limonciello (`superm1@kernel.org`).
Patches and bug reports welcome.

---

## How it writes

Two paths, chosen automatically:

1. **Direct sysfs** — used when the attributes are writable, i.e. running as
   root or as a member of the `halo-lp` group. The files are `root:halo-lp` mode
   `0664`.
2. **Halo Center REST API** — the fallback, at
   `http://127.0.0.1:10000/api/led`. Served by `halo-lp.service`, which runs as
   the `halo-lp` user and lists both attributes in its `ReadWritePaths`, so it
   performs the write on your behalf. This is why the script needs no `sudo`.

The API mirrors the same model:

```sh
curl -s http://127.0.0.1:10000/api/led
# {"color":{"R":0,"G":100,"B":0},"brightness":80}

curl -X POST http://127.0.0.1:10000/api/led \
  -H 'Content-Type: application/json' \
  -d '{"color":{"R":0,"G":100,"B":0},"brightness":80}'
```

If `halo-lp.service` is stopped *and* you lack sysfs write access, the script
exits with an explanation rather than failing silently.

---

## Troubleshooting

| Symptom | Cause |
|---|---|
| Colour lost after a reboot | Expected — nothing reapplies it. Run `sudo halo-led persist`. See [Surviving a reboot](#surviving-a-reboot). |
| Colour still lost after `persist` | Rule did not fire. Check `/etc/udev/rules.d/99-halo-led.rules` exists and that `LED_DIR`'s basename still matches `KERNEL==`. |
| LED unexpectedly white or full brightness | `brightness=0` was written somewhere (likely the dashboard). Re-set a colour. |
| `no write path` error | `halo-lp.service` is down and you are not root / not in `halo-lp`. Use `sudo`, or `systemctl start halo-lp`. |
| Colour set but LED looks dark | Brightness is very low, or the mix is `0 0 0`. Check `halo-led status`. |
| Colour looks washed out / wrong | Values were probably given as 0–255. Divide by 2.55. |

Inspect the raw state at any time:

```sh
cat /sys/class/leds/amd_halo:multicolor:status/multi_intensity   # "R G B"
cat /sys/class/leds/amd_halo:multicolor:status/brightness        # 0-100
cat /sys/class/leds/amd_halo:multicolor:status/multi_index       # "red green blue"
cat /sys/class/leds/amd_halo:multicolor:status/trigger           # expect [none]
```
