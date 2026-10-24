# Rank review-v3 incremental correction

Review-v3 is an exact overlay on the frozen review-v2 source. It copies all six v2 files unchanged, then changes only the three files listed in `replacement-contract.yaml`. Use `replacement.diff` to apply v3 atop v2; do not replace v2 with a partial patch.

It adds an explicit participant-avatar retry entry point intended for calls after host backoff, includes the complete UFont header in the base translation unit, and resolves tab text through the inner button's content. Successful avatar cache/image mapping remains keyed by participant id. No image or production file was changed.
