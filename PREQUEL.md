# PREQUEL — how this library got here

The public git history of `react-native-dl-scan` begins at a single squashed
root commit. That is deliberate: the project was built and tuned by scanning a
**real** driver's license on real devices, and the original ~12-month history
accumulated that real PII (names, DOB, license number, address) in test
fixtures, OCR dumps, and commit messages. Rather than play whack-a-mole
rewriting 170 commits, the history was collapsed to one clean commit and all
test identity values were made fictional (see [`.env.example`](.env.example)).

This document preserves what the squash threw away: **the story of how the
library evolved, and — more usefully — the dead ends it walked down first.**
The project was developed with AI coding assistants (Claude Code, Codex/GPT);
that collaboration is part of the record and is intentionally credited here.

---

## The arc (June 2025 → June 2026)

**Phase 0 — from a template.** It started as the stock React Native library
template (the one whose example function is `multiply()`). The first real work
replaced that scaffold with license-scanning infrastructure.

**Phase 1 — the iOS-only build-out.** A long, sprint-driven phase produced an
iOS TurboModule scanner: AAMVA parsing via the `DLParser-Swift` CocoaPod, a
React Native bridge, Vision Camera **v3**, a hand-written PDF417 frame
processor, Apple Vision OCR, heuristic document detection, a fallback
controller, the `useLicenseScanner` hook, a result UI, accessibility, and CI.
This phase was dominated by **test-infrastructure instability** — multiple
"emergency stabilization" rounds to get the suite reliably green. It shipped as
**v0.2.0, the first public release**.

**Phase 2 — the cross-platform rewrite.** The pivotal decision: stop being an
iOS-only TurboModule and become a **Nitro Modules** library backed by a **shared
C++17 core** (AAMVA parser, OCR field extractor, multi-frame voter), bridged
once to Swift and once to Kotlin so iOS and Android run the *same* parsing code.
Vision Camera went v3 → **v5**. The redundant `DLParser` pod and the
Swift-only AAMVA parser were absorbed into the C++ core. An on-device ML stack —
a field detector plus document rectification — was trained on the synthetic
**IDNet** dataset (no real PII in training).

**Phase 3 — refinement.** Multi-frame voting with per-field confidence tiers;
an iOS-26 `DataDetector` cross-check folded into a finalization pass; per-line
OCR observations surfaced to the API; images-only capture; an example app with
overlay/raw/original card views; per-region OCR performance work; and a final
licensing/namespace cleanup before going public.

---

## Dead ends and course corrections

The interesting part. Most of these cost real time and are worth not repeating.

- **`DLParser-Swift` CocoaPod → shared C++ parser.** Two AAMVA parsers lived in
  the call path at once. The C++ core absorbed both so there's one parser for
  both platforms.

- **iOS-only TurboModule → Nitro + C++ core.** The TurboModule had stub methods
  and no Android story. Rewriting on Nitro with a shared core let Android ship
  in the same push instead of doubling the work.

- **YOLOv8 / Ultralytics detector (AGPL) → NanoDet-Plus-m (Apache-2.0).** The
  field detector was originally YOLOv8n. Ultralytics' AGPL license is a
  non-starter for a permissively-licensed library, so the detector was retrained
  with `RangiLyu/nanodet` (Apache-2.0) and exported via `litert-torch`. The last
  pre-public commit ripped out the residual YOLO/Ultralytics tooling entirely.

- **Char-level disambiguation model (Keras) → dropped.** A model to resolve
  ambiguous OCR characters (`0/O`, `1/I/l`, `S/5`, `B/8`) was trained and
  evaluated, then cut as **net-negative** — it didn't earn its complexity. A
  postmortem was kept; the training data was preserved.

- **VLM as a second-tier parser (PaliGemma 2 3B) → killed.** Evaluated as a
  fallback for hard documents and abandoned: the synthetic data's document-type
  coverage was too narrow to trust a VLM tier without far more jurisdictions.

- **Android per-region OCR → dead end.** On iOS, cropping to a detected field's
  region (Vision `regionOfInterest`) improves reads. The Android ML Kit
  equivalent regressed across *many* iterations versus just OCR-ing the whole
  card. Android stayed on whole-card OCR.

- **Native orchestration of the TFLite runtime → JS orchestration.** Threading
  the `fast-tflite` model object through the Swift/Kotlin Nitro layer failed at
  both the C++ include and the Swift-protocol boundary (proven over a multi-cycle
  iOS build). The pipeline was split so JS/worklets load the models and call the
  tested C++ pre/post-processing through plain ArrayBuffer methods.

- **Hand-linking LiteRT into C++ → `react-native-fast-tflite`.** An early plan
  to write a C++ TFLite interpreter was correctly never built; the Nitro-based
  `fast-tflite` package replaced it and unified the runtime across both
  platforms.

- **`onnx2tf` for the NanoDet export → `litert-torch`.** The ONNX→TF conversion
  path was a dead end for this graph; the PyTorch→LiteRT exporter worked.

- **Core ML default int8 quantization → head-protected asymmetric.** Naive
  symmetric int8 quantization destroyed the model's accuracy. A head-protected
  asymmetric scheme ships at effectively FP16 accuracy with a large size cut.

- **A subtle export bug worth remembering:** the NanoDet export initially shipped
  raw class **logits** while the C++ decode expected **probabilities** — the
  fix was to bake the sigmoid into the exported graph (before quantization, which
  also bounded the values and made int8 viable).

- **Infrastructure tax.** Training repeatedly died to an unstable external drive
  (two lost runs and a kernel panic); the proven model was rescued to internal
  storage. A separate Android crash — a Nitro `ArrayBuffer` cursor being left at
  its limit, yielding a zero-length read and a `SIGSEGV` in preprocessing — only
  reproduced on a physical device.

---

## Where it landed

At the squash point the library is: a **Nitro HybridObject** over a **shared
C++17 core** (AAMVA + OCR field extraction + a multi-frame voter), with a
**NanoDet-Plus-m** field detector and a **DocAligner** rectifier — both
Apache-2.0 — run through **`react-native-fast-tflite`** and orchestrated from
JS/worklets. OCR is Apple Vision on iOS and ML Kit on Android; results are
voted across frames with provenance-aware confidence tiers, and on iOS 26 a
`DataDetector` pass cross-checks the dates. Both platforms are device-verified.

Test identity values are now fictional by default and overridable from a
gitignored `.env`, so a contributor can validate against their own card without
anyone's real license data ever entering git again.
