# Launch hook / shell QA: qa3d, vettv/qa-payload-3d 9a64ae55ca
Image: 3db4bcb009 + shell 0c12af82e0 + vetvdm 3d 58d7b86a13. Times CDT.
Hashes verified (copies in ../../iso/qa3d/): bootcd 70d83298…44a4 OK, regtest de19c70a…e9b8 OK, installer.img 7e890426…0fb4 OK, retroos-disk.img 9109e1c1…f282 OK. KEYECHO.COM 64246490…1fb7 OK. Installed vetvdm.exe sha f89c5b0a… = build output.
Method: QEMU 10.0.13 TCG pentium3 1024 MB, raw vm/qa-3d.raw, -rtc base=localtime, one case per boot, VMs qa-3d-*. Scripts vm/qa3d.sh, vm/qa3d-case.sh, vm/qa3d-boot.py (adds click + typing steps), qa3d/*.cmd. Lure + DosTest copied from vm/qa-3c.raw.

| # | Case | Result | Evidence |
|---|---|---|---|
| 1 | DOSBox-X missing → 0xE0000004 | PASS | dosbox-x dir renamed via mren (restored after). Shell: "The DOS/16-bit player isn't installed on this system. Details: C:\VetTV\logs\vetvdm.log" (vm/qa3d-b4nolayer-e5.png). shell.log `exit code=0xE0000004 seconds=1`. vetvdm.log `kind=dos layer=ntvdm exit=0xE0000004 duration_ms=1170 … warn=layer-fallback,job-nested,ntvdm-failed:err=0,exit=0x00000001` |
| 2 | Jazz → DOSBox-X | **FAIL** | From shell: shell passes `--kind=win16` (library tags Jazz 16-BIT), vetvdm+otvdmw run, otvdm "Interrupt 0D #GP (11E7:046C)" (vm/qa3d-b2jazz-e40.png). Also direct `vetvdm.exe C:\Games\Jazz\JAZZ.EXE` with no --kind (boot b3jazzd): vetvdm+otvdmw in every tasklist, no dosbox-x. JAZZ.EXE NE at 0xE3E, ne_exetyp=1, so the 3d classify rule should match. Not working on the image; for Integration (check that the header buffer covers lfanew+0x37 and the kind order in resolve_kind). The shell also needs to stop forcing win16 for these |
| 3a | SkiFree from shell + quit combo | PASS | vm/qa3d-b1ski-e25.png, no console button. shell.log `quit combo: vetvdm terminated` / `exit code=0xE0000008 seconds=25`. vetvdm+otvdmw gone at the next tasklist |
| 3b | H15c (no console) | PASS | SkiFree, Lure: no console window or taskbar button |
| 3c | Quit combo (Jazz, Lure) | PASS | Jazz `exit code=0xE0000008 seconds=41`. Lure `exit code=0xE0000008 seconds=76`, shell "Played Lure for 1 min." vetvdm+dosbox-x gone after |
| 4a | Lure keys | FAIL / not a valid key test | Lure went fullscreen. Space ×2 with no click, then click+Space, Enter, left click: title screen never changed (vm/qa3d-b5lure-*.png). Alt+Tab/Alt+Esc showed no Error window (Lure fullscreen) |
| 4b | KEYECHO via vetvdm | PASS (keys reach DOSBox-X) | `vetvdm.exe C:\qa\KEYECHO.COM` (cmd, not shell). Typed abc123, clicked, typed xyz: DOSBox-X shows `abc123xyz` (vm/qa3d-b6keyvdm-alttab.png). Note: the boot-launched vettvshell (topmost) stayed on top of DOSBox-X, the click landed on the shell and started SkiFree |
| 4c | KEYECHO at Z:\> | PASS | vettvshell killed in guest, `dosbox-x.exe -conf C:\qa\z.conf` (autoexec mount d C:\qa only). The first keys went to a modal **"ReactOS - no 3D acceleration"** box ("ReactOS DirectX Needs at least OpenGL 2.0 or better. Install a display driver that provides an OpenGL ICD to enable 3D acceleration. Or install Mesa3D from RAPPS"), Enter closed it. Then `D:\KEYECHO` typed at the prompt and ran, abc123 echoed (vm/qa3d-b7keyz-*.png). First try without -conf showed DOSBox-X's "Select folder…" first-run dialog (vm/qa3d-hung/b7try1-*) |
| 5 | USB EHCI + xHCI | PASS | Both: `ERASE sda`, `VETTV-INSTALL-OK /dev/sda 9109e1c1…f282`, the target boots on AHCI to the launcher, clock Central (17:20 / 17:37). vm/qa3d-usb/ |

Likely "Error" window: the ReactOS "no 3D acceleration" message box, raised when DOSBox-X tries OpenGL output. It's modal and takes the keyboard until dismissed. Suggest output=surface/ddraw in vetvdm's conf (opengl blocking is planned for 3e).
Flakes: boot 1 New Hardware Wizard (ACPI device) blocked the shell; boot 2 hung desktop (clock frozen), retried OK (vm/qa3d-hung/).
Disk kept: vm/qa-3d.raw (+ vm/qa-3d-base.raw). USB target qcow2s deleted. No qa-3d VM running.
