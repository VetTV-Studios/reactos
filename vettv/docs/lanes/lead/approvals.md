# Approval requests (RetroOS Lead → Vet)

Nothing below has been run. Each block runs only after Vet approves **that block by ID**. Pre-checks (read-only, Fri Oct 2, 2026, 2:08 PM PT): `gh` logged in as `Vet-TV` with `repo` + `workflow` scopes; `Vet-TV/reactos` and `Vet-TV/dosbox-x` do not exist; upstream default branch is `master` for both.

## A1 — Fork ReactOS and create `vettv/main`  *(status: DONE Fri Oct 2, 2026 ~3:12 PM PT; vettv/main = d004b2c119ef54a8cd292801d35654cec49195ca, verified by Lead)*
Effect: creates the public repo `github.com/Vet-TV/reactos` (fork of `reactos/reactos`, all branches) and one new branch `vettv/main` pointing at upstream `master`. No code is pushed. GitHub disables Actions on new forks by default; they stay off until A3.

```bash
gh repo fork reactos/reactos --clone=false
# wait for the fork to finish provisioning, then:
SHA=$(gh api repos/Vet-TV/reactos/git/ref/heads/master --jq .object.sha)
gh api -X POST repos/Vet-TV/reactos/git/refs -f ref=refs/heads/vettv/main -f sha="$SHA"
```
Verify: `gh repo view Vet-TV/reactos --json parent,defaultBranchRef` and `gh api repos/Vet-TV/reactos/branches/vettv/main --jq .commit.sha` (must equal `$SHA`).
Undo: `gh repo delete Vet-TV/reactos` (separate approval; needs the `delete_repo` scope, which the token does not have).

## A2 — Fork DOSBox-X  *(status: NOT recommended yet)*
The brief says fork only if we need patches; M1 consumes pinned upstream release artifacts plus the matching source tarball. If we later need patches:

```bash
gh repo fork joncampbell123/dosbox-x --clone=false --default-branch-only   # master only, skips main-osfree
```
Recommendation: defer until DOSBox-X Integration shows a concrete patch we need.

## A3 — Enable CI on the fork  *(status: DONE 7:09 PM PT. vettv/main fast-forwarded to 517dfe7b99. Run 37088729334 (VetTV Build) in progress. Lead-verified.)*
Status: drafting. This waits on (a) a clear M0-8 screenshot showing the full version string and (b) Build & Kernel's final commit SHA and diff stat. Then I ask Vet in DM.
Commit (local, verified as a fast-forward from fork/vettv/main): `517dfe7b99b04cb71d2a030206e65b10adbb1102`. Diff: 5 files, +103/-5 (vettv-build.yml +90, .github/vettv/diff-size.sh +8, plus the M0-7 branding). 112 lines differ from upstream outside modules/vettv/, under the 500 limit.
Sequence (Build & Kernel's revised order, accepted by Lead at 5:22 PM PT), with no force-push:
1. Enable Actions: `gh api -X PUT repos/Vet-TV/reactos/actions/permissions -F enabled=true -f allowed_actions=all`
2. Disable the inherited workflows: `gh workflow disable build.yml -R Vet-TV/reactos`, then the same for `labeler.yml` and `stale.yml`
3. `git push fork 517dfe7b99b04cb71d2a030206e65b10adbb1102:refs/heads/vettv/main`. This is a fast-forward only, and the push on `vettv/**` starts the first run.
4. Watch it with `gh run watch -R Vet-TV/reactos`. No workflow_dispatch is needed, because that only works from the default branch (master). Changing the default branch is A4.
Rollback: `gh api -X PUT repos/Vet-TV/reactos/actions/permissions -F enabled=false`. vettv/main only moves forward, and older commits stay reachable.

## A4 — Set fork default branch to `vettv/main`  *(status: optional, after A1)*
```bash
gh repo edit Vet-TV/reactos --default-branch vettv/main
```

## R-GATE — Real Reboot: allow boot-config writes on a real machine  *(status: CLOSED, not requested yet)*
Nothing in the Real Reboot project may run `bcdedit`/firmware-variable/partition/BitLocker commands on a real machine until this is signed off by Lead **and** Vet. Preconditions are listed in `companion/TASKS.md` (Safety gate). The first real target would be the Asus only; the HP 255 G10 is never a target.

## A5 — Ask CSMWrap upstream about the licence version  *(status: DRAFT, not requested, low priority, P4 only)*
- What: open one GitHub issue on `CSMWrap/CSMWrap`, posted publicly as Vet-TV, asking whether "LGPL-2.1" means "2.1 or later", since the bundled SeaBIOS code is LGPL-3.0.
- Why: shipping CSMWrap in Real Reboot needs a compatible licence. Until then it's test-only.
- Command (after approval only): `gh issue create -R CSMWrap/CSMWrap --title "Licence: LGPL-2.1-only or -or-later?" --body-file lanes/lead/drafts/a5-csmwrap-issue.md`
- I'll ask Vet when Real Reboot reaches the release-prep stage, not now.

- 2026-10-02 8:28 PM PT: Vet approved the guard-fix push in Build & Kernel's 1:1. `vettv/main` was fast-forwarded 517dfe7b99 → e59ac077a5 (Lead verified read-only). VetTV Build run 37093271193 is in progress.

## A-ALL — Standing approval from Vet  *(status: ACTIVE since 2026-10-18 11:44 PM CT)*
Vet, in the RetroOS Team room: "This is my go ahead to complete any and all tasks that require my permission."
How Lead applies it (Lead logs every remote write here before or right after it happens):
- Covered: pushing to vettv/* branches, opening and merging PRs into `vettv/main` (only after the owner review **and** a QA pass), running the VetTV Build Actions workflow, launching and steering Cursor cloud agents, upstream syncs.
- Still never: force-push, Microsoft files, copyrighted games/ROMs/BIOS, piracy, uploading Vet's game files.
- Held for a separate yes from Vet even so: R-GATE (boot-config writes on a real machine), public posts or issues on other people's repos (A5), and public GitHub releases.

- 2026-10-27 00:19 CT: Vet gave R-GATE yes for the Asus qa3f install (CSM on, clock set to Central, ERASE sda on the M.2). Steps: lanes/compat-qa/runs/asus-m2-install/STEPS-DRAFT-qa3f.md, installer sha256 4426e704...c4f8.
