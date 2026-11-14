# CHARACTER BIBLE — AfghanistanFPS
Maintained since: 2026-11-14 · Status: living document · Approval authority: project owner (with ChatGPT review)

Rules for this document:
- Every character entry separates **fictional design choices** from **historical claims**.
- Historical claims must carry a status label (VERIFIED / PLAUSIBLE / UNCERTAIN / UNSUPPORTED) and point at `Docs/Characters/<ID>/` research.
- No character design is "approved" without an explicit owner decision recorded here.

---

## GROM_01

| Field | Value |
|---|---|
| Character ID | **GROM_01** |
| Faction / viewpoint | Soviet-side playable character (fictional viewpoint) |
| Role | Fictional playable viewpoint — internal production name only; the name echoes the historical KGB "Grom" group but asserts no real unit identity |
| Historical reference period | Late 1979, Kabul (subject to ongoing research — see Research Brief section B) |
| Development stage | **RESEARCH** |
| Model status | **NOT STARTED** |
| Rig status | **NOT STARTED** |
| Animation status | **NOT STARTED** |
| Unreal integration | Existing player placeholder only (BP_PlayerFoundation + SKM_Manny_Simple); no character mesh connected |
| Design approval | **PENDING** — owner review of `Docs/Characters/GROM_01/GROM_01_Research_Brief.md` required (see its section G for the six pending decisions) |

### Design notes (from research brief)
- Recommended starting look: December-1979 Kabul **civilian disguise configuration** — pakol, perahan tunic, chapan, plain boots, minimal concealed gear (AKMS in a roll, suppressed PB pistol). Variant B (Afghan army fatigues + white armband, night-assault look) documented as a future optional skin.
- Placeholder policy: no dedicated crouch/death animations exist for the player character yet (documented in Phase 1); these carry forward to this character's pipeline.
- Prohibited content: invented unit insignia, anachronistic equipment (AS Val etc.), depictions of real named persons.

### Historical research
See `Docs/Characters/GROM_01/GROM_01_Research_Brief.md` (sections B–E carry the status labels; D carries the sourced reference board).

---

## Template for new character entries
Copy the GROM_01 table, keep every status field explicit, and link a per-character
research folder under `Docs/Characters/<ID>/`.
