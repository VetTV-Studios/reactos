# VetTV Retro OS project docs (portable snapshot)

Text-only copies of the project's working docs, taken from `/workspace/vettv-retro-os` on **2026-10-27**. They are snapshots; the live files on the shared box may be newer. Relative paths are preserved.

- `BRIEF.md`, `TASKS.md`, `companion/TASKS.md` – project brief, milestones and task queue.
- `lanes/<lane>/log.md` – per-lane work logs (build-kernel, compat-qa, dosbox-x, lead, shell-ux).
- `lanes/lead/approvals.md` – record of Vet's approvals.
- `lanes/compat-qa/runs/asus-m2-install/` – ISSUES-*, STEPS-DRAFT-* and the Asus PCI id dump (`pci-ids-qa3f.txt`).
- `lanes/compat-qa/runs/launch-hook/RESULTS-*.md` – launch-hook QA results.
- `lanes/dosbox-x/` – license audit, dependency pins (`deps.lock`), vetvdm spec, build research.
- `**/SHA256SUMS*`, `IMAGES-SHA256SUMS.txt` – hash lists for build/run artifacts (artifacts themselves not included).
- `lanes/*/cleanup-*.txt` – disk cleanup logs.

Not included: images, VMs, videos, binaries, game files, downloads/payloads, and anything Microsoft or copyrighted. qa3g results are pending and will be added later.
