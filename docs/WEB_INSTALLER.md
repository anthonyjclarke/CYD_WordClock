# Web installer

CYD_WordClock ships through the shared
[cyd-web-installer](https://github.com/anthonyjclarke/cyd-web-installer)
tooling: an ESP Web Tools page at
<https://anthonyjclarke.github.io/CYD_WordClock/>, built and published by
`.github/workflows/firmware.yml` on a `v*` tag on `main`. The design and the
rules behind it are in the pilot, CYD_AnimatedPixelClock
(`docs/WEB_INSTALLER_PLAN.md`).

---

## Project facts

| Item              | Value                                      |
|:------------------|:-------------------------------------------|
| Env               | `wordclock_cyd` – CYD 2.8″ ESP32-2432S028R |
| Platform          | `espressif32@6.12.0` (arduino-esp32 2.0.17) |
| `PROJECT_NAME`    | `CYD_WordClock` – frozen                   |
| Partitions        | Standard dual-OTA, `app0` 0x1C0000         |
| App size (1.2.0)  | 968,704 B – 53% of the slot                |
| Manifest parts    | `0x1000 / 0x8000 / 0xe000 / 0x10000`       |
| Improv name       | `WordClock-XXXX` (last 4 MAC hex digits)   |
| Setup hotspot     | `CYD-WordClock`                            |
| Secrets / FS / OTA | None / none / none                        |

---

## Hardware tests (Phase 5)

Run 09-10-2026 on macOS Chrome against the CI `site-preview` of `1.2.0-dev`
(run 37846122606), served from `localhost:8011`. Board: CYD 2.8″,
ESP32-D0WD-V3 rev 3.1, MAC `b0:cb:d8:da:ae:8c`, CH340 at
`/dev/cu.usbserial-110`.

| #   | Case                               | Result                                  |
|:----|:-----------------------------------|:----------------------------------------|
| 1   | Fresh install, erased              | Pass – see below                        |
| 2   | Update on provisioned board        | Pass – Update offered, settings kept    |
| 3   | Board on `app1`                    | N/A – no OTA in this firmware           |
| 4   | Wrong board                        | N/A – one env                           |
| 5   | `*-firmware.bin` via web `/update` | N/A – no `/update` page                 |
| 6   | macOS Chrome                       | Pass – cases 1 and 2                    |
| 7   | Windows Edge                       | Not run (optional)                      |

**Case 1.** Installed from the page with erase, WiFi set through
**Configure WiFi** (run at the bench). Boot log read afterwards: `Improv: listening on
Serial as WordClock-CBB0`, `v1.2.0-dev`, `Running from app0`, WiFi connected,
NTP synced, free heap 174,512 B.

**Case 2.** `brightness_default` set to 120 through `/api/config` (a
long-press brightness change is not saved, so it can't show survival). Board
flashed from PlatformIO as `1.2.0-dev.0`, an uncommitted edit since reverted.
Connect offered **Update CYD_WordClock** with no erase question. After the
Update: `v1.2.0-dev`, `Running from app0`, WiFi reconnected with no setup,
`brightness_default` still 120. The value was then put back to 180.

**Release check (v1.2.0, 09-10-2026).** Run 37861101957 published the release
and Pages; `SHA256SUMS.txt` verifies, and the Pages `firmware.bin` matches the
release asset. On the same board (then `1.2.0-dev`), the live page offered
**Update CYD_WordClock** with no erase question. Afterwards `/api/state` reported
firmware `1.2.0`. Boot log: `v1.2.0`, `Running from app0`, WiFi connected
from saved credentials at the same IP, NTP synced.

**1.2.1 patch – case 2 re-run (10-10-2026).** Vendored `lib/ImprovWiFi`
re-copied from cyd-web-installer `efe7cbd`: each Improv packet now starts on a
new line, because ESP Web Tools' Improv SDK drops a reply that follows noise on
port open, and Connect then intermittently offered Install. Same board
(MAC `b0:cb:d8:da:ae:8c`, which had since run CYD_BusStop_NSW). Flashed from
PlatformIO as `1.2.1-dev.0`, an uncommitted edit since reverted, then
`brightness_default` set to 120. From the CI preview of `1.2.1-dev` (run
38025291782), Connect offered **Update CYD_WordClock** with no erase question.
After the Update: `v1.2.1-dev`, `Running from app0`, WiFi reconnected with no
setup, `brightness_default` still 120. The value was then put back to 180.
