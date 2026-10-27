# Installing VetTV Retro OS on the Asus GL552VW (qa3f USB stick)

Image: qa3f (`vettv/qa-payload-3f` @ 51fd9488e5). QA-passed 2026-10-26 (see `lanes/compat-qa/runs/launch-hook/RESULTS-qa3f-51fd9488e5.md`).

> **Before you start, read this:**
> - **Vet must give an explicit yes, each time, for every BIOS or boot-setting change and for the install itself.** Nothing in this file is that yes.
> - **Recheck the installer hash against the final hash Build posts after the PR merge.** The hashes below are the qa3f build. If Build's final hash is different, use Build's file and hash, not these.
> - The install **erases the whole target drive**: the 512 GB M.2 SATA drive in the Asus. It's the RetroOS test drive, with no Windows on it. Nothing on it is kept.
> - Never use the HP 255 G10 or the Crucial X9 Pro for any of this.
> - **Flashing:** Rufus or Etcher can flash the `.xz` directly. Win32 Disk Imager can't, so decompress it first (7-Zip) and check the `.img` hash before flashing.

## Hashes (qa3f)
```
4426e70492f3cab493194630dd426d0722cdbb1a0a607109edd876dfa3ecc4f8  vettv-retroos-installer.img
248b5213d0eb31a05d9856c36a9f3516cd4443998dac8ff8ea2a1bba00f0b84e  vettv-retroos-installer.img.xz
```
After install, the installer prints the written disk's hash: `a97e569a88553164fbfc05045c2f79604f6a8c405b95285932f8a222f5021b7a`.

## A. Make the stick (on the MSI; any USB stick of 1 GB or more, it gets erased)
Download either `vettv-retroos-installer.img` or the smaller `vettv-retroos-installer.img.xz`, plus `SHA256SUMS`.

**Check the hash first. If it doesn't match, stop.**

**Windows 11**
1. In PowerShell: `Get-FileHash .\vettv-retroos-installer.img` (or `.img.xz`). Compare it with the matching line above and with Build's final hash.
2. If you downloaded the `.xz`, unpack it with 7-Zip to get the `.img`.
3. Open Win32 Disk Imager, pick the `.img`, and pick the **stick's** drive letter. Double-check that it's not an MSI drive. Click **Write**.
4. If Windows asks to format the disk afterward, click **Cancel**.

**Bazzite**
1. Run `sha256sum -c SHA256SUMS --ignore-missing`. Each file you have must say OK.
2. In GNOME Disks, pick the stick, then ⋮ → *Restore Disk Image…*, and choose the `.img` or `.img.xz`.

## B. On the Asus. Each step marked ⚠ needs Vet's yes first.
1. With nothing changed yet, press F2 at power-on and **take a photo of every BIOS screen**. Leave without saving.
2. **SATA mode stays AHCI.** Don't change it.
3. ⚠ **Legacy (CSM) boot:** in BIOS → Boot, set **Launch CSM = Enabled**. This may need **Secure Boot Control = Disabled** first. Save and exit.
4. ⚠ **Clock:** in F2 → Main → System Date/Time, set the BIOS clock to **local Central time**, not UTC. RetroOS reads the clock as local time. Note the old value from your photos.
5. With the laptop off, plug the stick into the **black USB 2.0 port**.
6. Power on and tap **Esc**. Pick the stick's entry **without** "UEFI:" (that's the legacy entry).
7. The installer lists the internal drives. Check that the ~512 GB drive is shown, usually `sda`.
8. ⚠ **Install (erases the drive):** type exactly `ERASE sda`, using the name it shows, and press Enter. Pressing just Enter, or typing anything else, cancels and writes nothing.
9. Wait for **`VETTV-INSTALL-OK`** (a few minutes). Unplug the stick and press Enter. The laptop powers off.
10. Power on. RetroOS boots to the VetTV launcher. If another OS starts, press Esc and pick the M.2 drive.

## C. First boot: what to expect
- In testing no New Hardware Wizard appeared, but real hardware may still show one. If it does, press **Esc** to close it (or click Cancel). The launcher stays in front.
- The launcher covers the whole screen, including the taskbar. If you leave it with F10, the desktop shortcut brings it back.
- The launcher clock should show **Central time**.
- The library is empty at first. Put each game in its own folder under `D:\Games` and press F5. Use only games you own, freeware, shareware or demos.

## D. Known gaps in qa3f (not blockers)
1. Lure ignores keyboard input. That's a problem with this game only.
2. In test VMs, a desktop sometimes froze on boot; a reboot fixed it. Report it if it happens.

## E. If something goes wrong
Every fix below needs **Vet's yes** first. Undo it afterward using the BIOS photos.
- **The installer says "no internal drive found":** stop and report. Don't switch SATA to IDE without a separate yes.
- **The stick isn't in the Esc menu:** check step B3 (CSM). If it still isn't there, stop and report.
- **Anything else:** stop, photograph the screen, and report. Change nothing else.

## F. What to photograph or report back to QA
- The BIOS screens before any change, and after each change.
- The hash result from step A.
- The installer's drive list (B7) and the `VETTV-INSTALL-OK` line with its hash (B9).
- The first-boot screen, including the wizard if one appears, then the launcher. Include the clock and the taskbar.
- One game launch from the launcher: does it come to the front, do keys work without clicking, and does Ctrl+Shift+F12 quit back to the launcher?
- Any error box, with a photo.
- The files `C:\VetTV\logs\shell.log` and `vetvdm.log`, if you can copy them to a stick.
