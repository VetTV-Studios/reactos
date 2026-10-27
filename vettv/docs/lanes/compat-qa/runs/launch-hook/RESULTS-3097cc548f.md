# Launch hook / vetvdm-core QA — rev 3b, vettv/qa-payload 3097cc548f
ISOs (sha256 re-verified 2026-10-26): bootcd aaacbcbd…3e, regtest 6fe40b8f…94. QEMU 10.0.13 TCG pentium3 1024MB IDE raw (vm/qa-r3b.raw), VMs qa-r3b-s1..s6.
Scripts: r3b/qar3b{,2,3,4}.cmd. Per-case logs copied in guest to C:\qa\v\ (vetvdm.log is rewritten per run, known bug). Launches via hook from cmd, not via the shell.

| Test | Result | Evidence |
|---|---|---|
| H18 payload layout | PASS | vetvdm.exe in System32; dosbox-x.exe, otvdmw.exe, vettvshell.exe under C:\VetTV; no C:\Windows\VetTV, C:\System32, C:\vetvdm.exe, C:\explorer.exe |
| WineVDM INI non-empty | PASS | C:\VetTV\winevdm\WINDOWS\SYSTEM.INI and WIN.INI are 61 bytes each (placeholder comment). They live in WINDOWS\, not the winevdm root |
| H15a exit code 3 | PASS | el=3; `caller=hook kind=dos layer=dosboxx exit=3 duration_ms=7858` |
| H15b exit code 0 | PASS | dos0.exe (synthetic MZ, int21 4C00) el=0, `exit=0` |
| H15c no console from shell/Explorer | NOT RUN | shell launch is blocked by the job bug |
| H15d cli without --from-hook | PASS (exit) | el=3, `caller=cli exit=3`. Console kept was not checked by eye |
| H15e lookup order | PASS | SKI.EXE `kind=win16 layer=winevdm wine="C:\VetTV\winevdm\otvdmw.exe" wine_from=winevdm-path`; SkiFree window up (vm/r3b-H15f-125521.png) |
| H15f VETTV_VDM_BYPASS no effect | PASS | still hooked, same line |
| H15g1 WineVdmPath valid custom | PASS (routing) | `wine="C:\qa\alt\otvdmw.exe"`. My fixture copied only otvdmw.exe, so it raised "Unable to locate" and the guest froze (test artifact) |
| H15g2 WineVdmPath missing | PASS (fixed vs 4a98e73) | `wine="C:\VetTV\winevdm\otvdmw.exe" wine_from=bundled` |
| H15h1 --kind=dos on SKI.EXE (expect WineVDM + warn=kind-override) | INCONCLUSIVE | vetvdm started via `start`, the guest stalled and no log line was written (the log is only written at exit). Rerun needed |
| H15h2 --kind=win16 on type5.exe | PARTIAL PASS | routed to DOSBox-X 2026.10.01 (mount C=C:\qa, V=temp, runs type5.exe; vm/r3b-H15h2-dosbox.png). The batch then stalled, so the warn=kind-override line was not captured |
| H15h3 --kind=dos on type5.exe | NOT RUN | stalled at H15h2 |
| H15i dosbox=/wine=/wine_from= fields | PASS | present with correct values in every captured line (a,b,d,e,f,g1,g2) |
| WineVDM recheck SkiFree via hook | PASS | H15e/f/g2 = C:\qa\SKI.EXE through the hook, otvdmw running in the tasklist. The C:\Games\SkiFree copy was not reached |
| WineVDM recheck Jazz via hook | NOT RUN | stalled before it (last result 2026-10-19: otvdm #GP) |
| A/B direct vs shell (0xE0000005) | DIRECT PASS / shell NOT RUN | direct cmd `vetvdm.exe C:\qa\dos3.exe` on the payload image → exit 3 via DOSBox-X (H15d), not E_START. On the payload image E_START is therefore not a direct-path failure. Build's 13:10 E_START on the default image comes from the missing DOSBox-X → NTVDM fallback. The job theory stays the lead explanation for the shell failure. The shell-side repro in the same boot was not reached |

Other observations
- `warn=locked-hostkey` appears on every DOS line.
- The log-overwrite bug is visible: each vetvdm.log has stale tail bytes (`n=locked-hostkey`, `-path`) after the new line.
- Guest stalls: 3 of 4 boots stalled in the batch about 1 minute after a `start` of a GUI layer (frozen screen at H15f, after the g1 dialog, after h1, during h2). Workaround: one case per boot, or a QMP-driven harness.

## Follow-up 2026-10-26 13:27-14:07 CDT (one case per boot, vm/case.sh + vm/cases.sh + vm/shellab.sh, VMs qa-r3b-c-*)
| Test | Result | Evidence |
|---|---|---|
| H15h1 --kind=dos on SKI.EXE | PASS | `kind=win16 layer=winevdm ... wine_from=bundled warn=kind-override` |
| H15h2 --kind=win16 on type5.exe | PASS | `kind=dos layer=dosboxx ... warn=kind-override,locked-hostkey`. exit=0xE0000009 only because killer.cmd force-killed DOSBox-X after about 9 min (the guest ping wait runs slow while DOSBox-X is up) |
| H15h3 --kind=dos on type5.exe | PASS | `kind=dos layer=dosboxx ... warn=locked-hostkey` (no kind-override). Same forced-kill exit |
| Jazz WineVDM recheck (hook) | FAIL (unchanged) | `kind=win16 layer=winevdm wine_from=bundled exit=1`, otvdm "Interrupt 0D #GP (11E7:046C)" (vm/r3b-jazz-otvdm-gp.png). Jazz's Borland RTM DPMI16 needs DOSBox-X routing or a profile override |
| A/B direct vs shell | CONFIRMED: shell-only failure | Direct hook: SKI.EXE and JAZZ.EXE reach WineVDM (75-90 s runs). Same image, same files, from vettvshell: SkiFree → "The DOS/16-bit player didn't start" (vm/r3b-shell-ski-fail.png); Jazz `caller=cli ... exit=0xE0000005 duration_ms=15`, shell.log `exit code=0xE0000005 seconds=0`. Consistent with the nested job theory. Build's default-image E_START is the separate NTVDM-fallback case |
| H15c | NOT RUNNABLE | blocked by the shell bug; rerun after the job fix |
| Log overwrite | CONFIRMED again | stale tail bytes after the new line in vetvdm.log and shell.log |
