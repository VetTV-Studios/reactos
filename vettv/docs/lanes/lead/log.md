# lead — log
- 2026-10-02 15:07 PT: Vet approved A1 (ReactOS fork + vettv/main). Relayed to Build & Kernel. Vet: Asus has Win10; owns DOS, Win3.1 and 9x games.
- 2026-10-02 15:09 PT: Vet has Win3.1 and Win95/98SE install media. Guest fallback + 86Box Win98SE reference are unblocked; media stays local to Vet's machines.
- 2026-10-02 15:11 PT: Vet confirmed all Win3.1/95/98SE media and games are owned copies.
- 2026-10-02 15:12 PT: A1 done and verified by Lead: Vet-TV/reactos, vettv/main d004b2c1.
- 2026-10-02 16:25 PT: Folded 3 companion projects into ROADMAP.md; M0 splits in companion/TASKS.md; R-GATE added (closed).

- 2026-10-19 9:00 AM CT: Vet moved the home repo to VetTV-Studios/reactos. All five vettv/* branches were pushed there and match Vet-TV/reactos (main e59ac077a5, launch-hook bd298ac19e, sync c4f739daba, vetvdm-core 4a98e7328e, vetvdm-paths a75d61e771). Upstream `Build` workflow disabled there (as on Vet-TV) and its 3 push-triggered runs cancelled; `VetTV Build` runs left going. Local remote `studios` added in src/reactos. Vet-TV/reactos stays as backup.
- 2026-10-19 11:37 CT: pushed vettv/launch-hook-rev6 (1c01e5ce3c, rev 6b GetBinaryTypeW/SCS_WOW_BINARY) to VetTV-Studios/reactos under A-ALL.
- 2026-10-19 12:01 CT: merged launch hook rev 6b into VetTV-Studios vettv/main via https://github.com/VetTV-Studios/reactos/pull/1 (QA pass RESULTS-72ef305c03, A-ALL).
- 2026-10-19 12:46 CT: copied vetvdm-core rev 3 98960d070e0 to studios (A-ALL).

- 2026-10-27 00:19 CT: Vet gave R-GATE yes for the Asus qa3f install (CSM on, clock set to Central, ERASE sda on the M.2). Steps: lanes/compat-qa/runs/asus-m2-install/STEPS-DRAFT-qa3f.md, installer sha256 4426e704...c4f8.

- 2026-10-27 00:20 CT: qa3f (51fd9488e5) merged via PR #4 into vettv/main, now d1b2c5159c. Vet approved the push.
- 2026-10-27 00:46 CT: Asus qa3f install OK and boots (g51fd948). Real-hardware issues: wizard for Other PCI Device, and the touchpad and mouse are dead (keyboard OK). Sent to QA and Build.
- 2026-10-27 01:32 CT: Vet set driver queue 3h xHCI, 3i audio, 3j RTL8168, 3k chipset+Bluetooth (after 3g merges).
