# Installing VetTV Retro OS on the Asus GL552VW (qa3g USB stick)

Image: qa3g (local `vettv/hw-3g` @ 10337654ef, desktop label g1033765). QA-passed 2026-10-27 (see `lanes/compat-qa/runs/launch-hook/RESULTS-qa3g-10337654ef.md`).

> **Before you start, read this:**
> - **Vet must give an explicit yes, each time, for every BIOS or boot-setting change and for the install itself.** Nothing in this file is that yes.
> - **Recheck the installer hash against the final hash Build posts after the PR merge.** The hashes below are the qa3g build. If Build's final hash is different, use Build's file and hash, not these.
> - The install **erases the whole target drive**: the 512 GB M.2 SATA drive in the Asus. It's the RetroOS test drive, with no Windows on it. Nothing on it is kept.
> - Never use the HP 255 G10 or the Crucial X9 Pro for any of this.
> - **Flashing:** Rufus or Etcher can flash the `.xz` directly. Win32 Disk Imager can't, so decompress it first (7-Zip) and check the `.img` hash before flashing.

## Hashes (qa3g)
```
b828f39cad57da1dcd7c9963e3fd00dd0d5e287eb161c552dea3dc9868607342  vettv-retroos-installer.img
336e7d7b5b86e941ca7d2789f1d2b4be6251a9df09b052ee2d0fcbefb1872779  vettv-retroos-installer.img.xz
```
After install, the installer prints the written disk's hash: `13b63e76ca9c9d0fcd39a770c51f590f4019855a983ecdc0c552bc843bbe09e2`.

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
- qa3g turns the New Hardware Wizard off, so it shouldn't appear. If one does, press **Esc** (or click Cancel) and photograph it. The launcher stays in front.
- The touchpad and the USB mouse may not work (see D). Use the keyboard.
- **Device Manager check:** press F10 for the desktop, then Win+R → `devmgmt.msc` → Enter. Check that no QEMU devices are listed and that no popups appear. Photograph the list, expanding any yellow "!" entries.
- **Careful:** after F10, don't press Enter on the desktop. The hidden launcher still takes the key and starts the selected game.
- The launcher covers the whole screen, including the taskbar. If you leave it with F10, the desktop shortcut brings it back.
- The launcher clock should show **Central time**.
- The library is empty at first. Put each game in its own folder under `D:\Games` and press F5. Use only games you own, freeware, shareware or demos.

## D. Known gaps in qa3g (not blockers)
1. **Sound: N/A.** There's no HDA driver yet (8086:A170, R-T05d, M-risk).
2. **USB mouse:** doesn't work on xHCI (8086:A12F, R-T05c, M-risk). Legacy USB Support is already on.
3. **Touchpad:** not supported (I2C HID, R-T05b).
4. **Wired network:** none for the RTL8111/8168 (10EC:8168, R-T05f, M-risk). A user-supplied driver is the only route for now.
5. **Wi-Fi:** none for the Intel 7265 (R-T05g).
6. Lure ignores keyboard input. That's a problem with this game only.
7. In test VMs, a desktop sometimes froze on boot; a reboot fixed it. Report it if it happens.
8. After F10, Enter on the desktop starts the selected game. Avoid it; it's for Shell/UX.

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
- The Device Manager list (C).
- One game launch from the launcher: does it come to the front, do keys work without clicking, and does Ctrl+Shift+F12 quit back to the launcher?
- Any error box, with a photo.
- The files `C:\VetTV\logs\shell.log` and `vetvdm.log`, if you can copy them to a stick.
