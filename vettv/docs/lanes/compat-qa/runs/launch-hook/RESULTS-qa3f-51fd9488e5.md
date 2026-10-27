# Launch hook / shell QA: qa3f, vettv/qa-payload-3f 51fd9488e5
Image: 003e0b6152 + shell 8c0e35fb82 + vetvdm e7917006af + wizard fix 65603ebdd0. Times CDT. Run 22:12–23:36 CDT 2026-10-26.
Method: same as qa3e (QEMU TCG pentium3 1024 MB, raw vm/qa-3f.raw, regtest unattended -no-reboot then -boot c, -rtc base=localtime, mtools @@1048576, QMP, one case per boot, VMs qa-3f-*). Scripts vm/qa3f*.sh, vm/qa3f-boot.py, qa3f/*.cmd. Guest logs in vm/qa3f-logs/<case>/.

| # | Check | Result | Evidence |
|---|---|---|---|
| 0 | Hashes | PASS | installer.img 315621376 B 4426e704…c4f8, .xz 248b5213…b84e, retroos-disk.img a97e569a…1b7a, .gz cb95d06f…c9ce, bootcd 3c6c7034…4f65, regtest b3e97145…35e8 (iso/qa3f/SHA256SUMS.txt). On installed disk: vetvdm.exe 208896 B 1946f4e7…51ea, vettvshell.exe 75264 B 52198030…f5f8 |
| 1a | DOSBox-X missing | PASS | b11nolayer (host mren, DosTest tile): "The DOS/16-bit player isn't installed on this system" (vm/qa3f-b11nolayer-e8.png); shell.log `exit code=0xE0000004 seconds=1`; vetvdm.log `caller=shell … layer=ntvdm exit=0xE0000004 … ntvdm-failed:err=0,exit=0x00000001`. Restored |
| 1b | SkiFree, H15c, quit combo | PASS | b1ski: SkiFree in front, no console; click at 400,300 went to SkiFree (vm/qa3f-b1ski-click.png); `quit combo: vetvdm terminated`, `exit code=0xE0000008 seconds=42`. Jazz s=40, Lure s=36 also 0xE0000008 |
| 1c | KEYECHO via vetvdm (cmd) | PASS | b6keyvdm vetvdm.log `caller=cli program="C:\qa\KEYECHO.COM" … layer=dosboxx exit=0` |
| 1d | KEYECHO at Z:\> | PASS | r7keyz: `d:`, `keyecho`, `abc123` all registered (vm/qa3f-r7keyz-typed.png). b7keyz boot hung at empty desktop (known flake), rerun clean |
| 1e | KEYECHO tile, no click | PASS | b5keytile: `abc123` incl. first key (vm/qa3f-b5keytile-typed.png); exit 0x00000000 after Esc |
| 1f | Jazz → DOSBox-X | PASS | Tile: `--caller=shell --kind=auto`, dosbox-x in tasklist, Jazz DOS setup screen, no #GP (vm/qa3f-b2jazz-e40.png). Direct: `caller=cli … JAZZ.EXE kind=dos layer=dosboxx exit=0` |
| 1g | F10 + taskbar click | PASS | b4f10 shell.log only `exit to desktop`; taskbar/Start clicks start nothing (vm/qa3f-b4f10-*.png) |
| 1h | No OpenGL box | PASS | None in any qa3f screenshot |
| 1i | Game to front | PASS | SkiFree, Jazz, KeyEcho, Lure |
| 2 | caller=shell | PASS | b5keytile `pid=1248 caller=shell program="C:\Games\KeyEcho\KEYECHO.COM" … exit=0`; b11 `caller=shell` |
| 3 | vetvdm_test + golden files | PASS | Build's vetvdm_test.exe 66660afa…a64d, golden files in C:\qa\golden: `206 checks ok`, `exit=0` (vm/qa3f-logs/b9test/v/unittest.txt) |
| 4 | USB EHCI + xHCI → AHCI | PASS | Both `VETTV-INSTALL-OK /dev/sda a97e569a…1b7a`. First AHCI boot: no New Hardware Wizard, no taskbar, launcher hint line fully visible ("No games yet…"), clock 22:29 / 22:51 Central (vm/qa3f-usb/qa-3f-usb-boot-*-desktop0.png, -desktop.png). (Alt+F4 sent by my script afterwards closes the launcher to the desktop, expected) |
| 5 | F10 → `vettvshell --activate` | PASS | b10act: after F10 (desktop), `--activate` brings the launcher back full-screen covering the taskbar (vm/qa3f-b10act-activated.png). Enter → SkiFree, quit combo → launcher back full-screen, hint "Back from SkiFree.", no taskbar (vm/qa3f-b10act-back2.png) |
| – | Lure | Known game issue | Launches in DOSBox-X via shell, quit combo OK |

Notes: the first boot of the freshly installed IDE regtest disk (b0warm) showed only the regtest cmd window and no launcher (no wizard this time); a warm-up boot was used, same as qa3e. One hung-desktop flake (b7), rerun clean.

Verdict: all checks PASS. qa3f is clean to replace Vet's stick.
Disks: kept vm/qa-3f-base.raw (fresh install + test files). Deleted vm/qa-3f.raw and USB qcow2s. No qa-3f VM running.
