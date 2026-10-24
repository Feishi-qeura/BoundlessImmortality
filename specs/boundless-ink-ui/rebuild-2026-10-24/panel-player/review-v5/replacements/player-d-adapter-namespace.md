# Player-d namespace replacement overlay

Keep `player-d/contracts/action-adapter.yaml` frozen. Use `../contracts/player-d-adapter-namespace-overlay.yaml` as its review-v5 overlay; only these two type references change:

```yaml
dispatch_delegate: InkPlayerCraftSourcesWidgets::FActionRequested
payload_type: InkPlayerCraftSourcesWidgets::FActionPayload
```

The existing header declares both types in `namespace InkPlayerCraftSourcesWidgets`. All page/action mappings and gates in the frozen Player-d contract are preserved.
