# Player E execution basis

Updated: 2026-10-24

## Authority and scope

This isolated candidate slice stages independent Breakthrough and Tribulation page composers. Continuous component/page authorization is recorded in `../quickstart.md`; final visual acceptance remains user-owned. Every artifact stays `approved:false`. No production `Source/` or `Content/`, shared canonical contract, or frozen earlier handoff is modified.

## Canonical inputs

Use `contracts/style-contract.yaml`, `contracts/screen-contract.yaml`, the `breakthrough` and `tribulation` screen briefs, `model/vm-contract.yaml`, `model/action-contract.yaml`, and `model/screen-bindings.json` as source of truth. Files in this directory only map and stage those shared definitions.

## Composition

- Breakthrough displays only actual stage, selection, quote, quality, optional forecast, and read-only InventoryVm data. Missing `q_normalized`, readiness, slots, sources, and growth range remain unknown. Quote is a query/preview and never authorizes execution. Prepare and rework require an explicit confirmation before the canonical command.
- Tribulation renders only the actual attempt phase and authority-provided timers, helpers, probability, outcome, and receipt. It never creates future phases or helper/roll data. Cancel and reconcile pass the actual attempt ID and are gated by host-provided action availability. No anchor-side Assist action is exposed.
- Both pages use independent trees, injected style tokens, 720×1280 portrait and 1280×720 two-column landscape composition, and host safe-inset/hit-target values.
- Existing visual candidates are layout inputs. This implementation uses real provider item icons if present and has no need for substitute/generated semantic art.

## Verification boundary

Static source and contract checks only. No Unreal build, automation, Editor, PIE, cook, package, device, or platform result is claimed.
