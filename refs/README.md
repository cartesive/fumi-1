# refs/ — reference material (not in git)

Everything in this folder except this file is ignored by git. It is for listening and measurement only;
nothing here is ever copied into the firmware or published.

Put here:

- The ST-50 recordings (the preset demo with mi = A, Hainbach's video with the clean opening run). The
  audition bench lists any audio file here under "Reference clips", with a from / to selection.
- The DX7 `.syx` bank that holds KOTO and HARP 2. The bench loads every `.syx` here ("refs/" button, or
  automatically when it starts through `web/bench/serve.py`); when it finds voices named KOTO and HARP 2 it
  makes them A and B and generates the first set of hybrids.
- The panel photo and the voice-strip crop.

Tools that read from here:

```
tools/analyze_ref.py refs/demo.mp3 --sections            # pitch per note, attack, click, decay, partials
tools/analyze_ref.py refs/hainbach.mp3 --from 0.45 --to 4.4 --peaks   # the ringing run: one spectrum
tools/syx_dump.py refs/bank.syx                          # the voices in a bank
tools/syx_dump.py --diff refs/bank.syx:23 refs/bank.syx:12   # what differs between two voices
```
