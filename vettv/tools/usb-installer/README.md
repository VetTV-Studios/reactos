# VetTV Retro OS — USB installer stick (QA build, qa3d)

A raw disk image (`vettv-retroos-installer.img`, also `.img.xz`) for a USB stick (1 GB or larger; everything on the stick is erased).
Booting it starts a small Linux (Alpine linux-lts kernel + busybox, no network) that:

1. checks the RetroOS image on the stick against its sha256,
2. lists the internal drives (the USB stick itself is never offered),
3. writes the **pre-installed** VetTV Retro OS disk to the drive **only after you type `ERASE sda`** (or whatever name it shows) and press Enter. Just pressing Enter cancels, and nothing is written,
4. reads the drive back and checks it, then prints `VETTV-INSTALL-OK`.

The pre-installed disk is the qa3d QA build (bootcd sha256 `70d83298…44a4`, branch `vettv/qa-payload-3d` 9a64ae55ca = vettv/main 3db4bcb009 + shell 0c12af82e0 + vetvdm rev 3d 58d7b86a13), with DOSBox-X, WineVDM and vettvshell. It's QA-only, not a public release. Hashes are in `SHA256SUMS` next to the image.

> **Heads up:** the target drive gets erased completely. On the Asus GL552VW the target is the 512 GB M.2 SATA test drive. The installer only uses the first ~4 GB; the rest of the drive stays unallocated.

## 1. Write the stick on the MSI

Download `vettv-retroos-installer.img.xz` and `SHA256SUMS`, and check the hash first.

### From Bazzite (Linux)
```
sha256sum -c SHA256SUMS --ignore-missing
```
- **Easiest:** open **GNOME Disks** (or KDE **ISO Image Writer**). Pick the USB stick, choose ⋮ → *Restore Disk Image…*, select the `.img.xz` (or the unpacked `.img`) and start the restore.
- **Terminal:** run `lsblk` to find the stick, for example `/dev/sdb`. Check the size and make sure it is **not** your internal drive. Then:
  ```
  xzcat vettv-retroos-installer.img.xz | sudo dd of=/dev/sdX bs=4M conv=fsync status=progress
  ```

### From Windows (Shadow PC can't see local USB, so use the MSI itself)
- Check the hash in PowerShell: `Get-FileHash .\vettv-retroos-installer.img.xz` and compare it with SHA256SUMS.
- Unpack the `.xz` with 7-Zip, then write the `.img` with a plain raw image writer such as **Win32 Disk Imager**: pick the `.img`, pick the stick's drive letter, click **Write**. Double-check the drive letter is the stick, not the MSI's own drives.
- If Windows offers to "format the disk" afterward, click **Cancel**.

## 2. Boot it on the Asus GL552VW

> **Clock:** set the Asus BIOS clock (F2 → Main → System Date/Time) to **local Central time**, not UTC. RetroOS treats the hardware clock as local time, and there's no Windows on this laptop, so local time is fine. Ask Vet before changing it.

1. With the laptop off, plug the stick into a USB port. The **blue USB 3.0 ports** work; if the stick isn't found, use the black USB 2.0 port.
2. Power on and tap **Esc** right away to open the one-time boot menu.
3. Pick the USB stick entry (shown with the stick's brand name; if there are two, pick the one **without** "UEFI:"). **No BIOS changes are needed.**
4. The installer shows the internal drives with their sizes. Check that the 512 GB drive is listed (usually `sda`).
5. Type exactly `ERASE sda` (using the name it shows) and press Enter. Writing and verifying takes a few minutes.
6. When it says **Done** with `VETTV-INSTALL-OK`, unplug the stick and press Enter. The laptop powers off.
7. Power on again. If it boots another OS, press Esc and pick the M.2 drive. RetroOS boots to the VetTV launcher.

### Only if the stick does not appear in the Esc menu
Enter the BIOS with **F2**. Under *Boot*, set **Launch CSM = Enabled** (this may need *Security → Secure Boot Control = Disabled* first). Save with F10 and try Esc again. Tell Build & Kernel before changing anything else. If the installer says "no internal drive found", the SATA mode needs to be AHCI. Report it rather than changing other settings.

## 3. How it's built (for the team)
```
./build-retroos-disk.sh <bootcd.iso> <outdir>                # unattended install in QEMU -> retroos-disk.img(.gz)
./build-installer-stick.sh <outdir> vettv-retroos-installer.img
./test-qemu.sh vettv-retroos-installer.img ehci <dir>        # and xhci: install onto a scratch disk, boot it
```
The scripts never touch `/dev` on the build box. The QEMU VMs are named `bk-usb-*`. The unattended install uses `unattend.inf` here (US English, Central time, 800x600x32, FAT32).

## 4. This build (2026-10-26, qa3d)
| File | sha256 |
|---|---|
| vettv-retroos-installer.img (315,621,376 B) | 7e890426fd6df23472ccec8f74dd1a9f70e30345613e5d4fbc645f739f940fb4 |
| vettv-retroos-installer.img.xz (113,004,296 B) | e8632289070cb9f74a79ede1d9a3f11a28890ee9dcb346615e696a6e65f1b165 |
| retroos-disk.img (written to the drive, 4000 MiB) | 9109e1c15eaae5c732e87c1da74dc27c37b3174700c002ac6f6e8c886140f282 |
| retroos-disk.img.gz | 177750588fb85968070f47ceb6e000acc99abd636a16039f44be3e233e8d08af |

Source ISO: qa3d bootcd 70d83298…44a4 (output-qa3d, -Werror; rebuilt with mkisofs from the same tree, only `i386\unattend.inf` differs; Central time, TimeZoneIndex=20). All test VMs run with `-rtc base=localtime`.
On first boot, a "New Hardware Wizard" may show in the taskbar. Cancel it, or just leave it.

The previous rev 3b files are kept next to these as `vettv-retroos-installer-r3b.img(.xz)` and `disk-r3b/` (stick c3ec6f6b…d42f, xz 51804eb5…a6e1, disk 6b71c9f3…3052); superseded. The unused rev 3s files are kept as `vettv-retroos-installer-r3s.img(.xz)` and `disk-r3s/` (stick 2a9f2a70…6898); superseded by qa3d, never handed out.

## In-repo copy (VetTV 3g)

Source of truth for these scripts. They are host-side tools only and are NOT part of the ReactOS CMake build.
Not committed here: images, `SHA256SUMS`, build outputs, any credentials.

Box-specific defaults, override with env vars:
- `build-installer-stick.sh`: `CACHE` (default `/workspace/bk-usb-cache`) holds the Alpine netboot tarball (pinned by SHA-256) and its unpacked modloop.
- `build-retroos-disk.sh` expects the ReactOS build dir next to the input ISO (`<build>/boot/bootcd.Debug.lst`, `<build>/host-tools/bin/mkisofs`).
- Needs: qemu-system-i386, qemu-img, sfdisk, mtools, libhivex-bin (hivexsh), socat, syslinux, dosfstools, squashfs-tools, xz.

Order: `build-retroos-disk.sh <bootcd.iso> <outdir>` (stages 1-3, then `cleanup-qemu-enum.sh` strips QEMU-only Enum keys from SYSTEM) -> `build-installer-stick.sh <outdir> <stick.img>` -> optional `test-qemu.sh`.
