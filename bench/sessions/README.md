# bench/sessions

Session files exported from the audition bench ("Export session"), one per round: every patch's name, source,
notes and rating, the A/B choice and the history. Patches that came from someone else's `.syx` (and anything
made from them) carry a fingerprint instead of their bytes, so no ROM patch data enters the repository; the
bench reattaches the bytes when that `.syx` is loaded again. FuMi's own patches travel whole.
