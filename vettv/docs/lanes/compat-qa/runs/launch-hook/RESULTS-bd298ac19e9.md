# Launch hook rev 5 `bd298ac19e9`: QEMU results, 2026-10-19 9:12 AM CT
ISO: Build's local bootcdregtest.iso 0fece3b1…6e57 (merge 313e66dc7c). QEMU 10.0.13 TCG, VM name qa-hook5-*.
Stub e615be4c… installed as System32\vetvdm.exe. Targets: dos3.exe (hand-made MZ, ac3fb62e), SKI.EXE (G011, real Win16 NE, from ski.zip 2d3da30c; local only),
launch.exe (9b753de5, Win32 CreateProcess launcher) copied as myotvdm/otvdm/otvdmw/VetVdm.exe to fake the caller name. Script: run2/qa2.cmd; per-test stub logs in run2/.

| Check | Result | Evidence |
|---|---|---|
| H1 off by default | PASS | no key, no stub line |
| H2 full path | PASS | 1 line, kind=dos, --image=C:\qa\dos3.exe, EL 7 |
| H3 bare name, cwd | PASS | --image full path, tail `dos3.exe` |
| H4 via PATH | PASS | --image=C:\qa\bin\dospath.exe |
| **H5 Win16 -> kind=win16** | **FAIL** | real NE SKI.EXE gets `--kind=dos` (fixture ne.exe too) |
| H6 cwd kept | PASS | %CD% unchanged; console look not checked (headless) |
| H7 loop guard (passthru) | PASS | 1 hook line + `passthru child exit=0`, no second line |
| H8 vetvdm missing | PASS | no line, normal path, no crash |
| H9 spaces + apostrophe | PASS | quoted --image and tail correct |
| H10 WineVDM re-entry | NOT RUN | no WineVDM in image; see H13b/H16c |
| H11/H16a otvdm caller + DOS | PASS | 1 line kind=dos |
| H16b otvdmw caller + DOS | PASS | 1 line kind=dos |
| H12 Win32 launcher + DOS | PASS | 1 line |
| H13a myotvdm caller | PASS | not skipped |
| **H13b OTVDMW caller + Win16** | **FAIL** | hooked (should skip), knock-on of H5 |
| **H16c otvdm caller + Win16** | **FAIL** | hooked (should skip), knock-on of H5 |
| H16d VetVdm.exe caller (mixed case) | PASS | skipped, went to NTVDM |
| H14 VETTV_VDM_BYPASS set | PASS | still hooked |
| H15 / H17 | NOT RUN | need vetvdm-core / vettvshell images |

Root cause (H5): ReactOS ntoskrnl/mm/section.c never returns STATUS_INVALID_IMAGE_WIN_16 or _NE_FORMAT;
an NE file comes back as STATUS_INVALID_IMAGE_PROTECT, so proc.c takes the DOS branch and the Win16 hook site never runs.
This predates rev 5 (same in every rev). Effect: Win16 games go to vetvdm as DOS, and once WineVDM is in, otvdm starting a Win16 child is re-hooked.
Side note: NTVDM failed dos3.exe ("start-app BOP"), which is a fixture problem, not the hook.
