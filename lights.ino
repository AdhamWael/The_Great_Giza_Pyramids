// =============================================================================
//  led_story_timeline.ino  —  Giza Pyramid LED Show
//  Synced to try3.mp3  (86.96 s)
//  Target board: Arduino Uno / Nano
//
//  HOW TO START THE SHOW  —  KEYBOARD TRIGGER (A + F)
//  ────────────────────────────────────────────────────
//  This sketch WAITS for a Serial 'G' byte before starting.
//  Nothing moves until the web page sends the go signal.
//
//  Step-by-step:
//    1. Upload this sketch to the Arduino.
//    2. Open led_show_launcher.html in Chrome or Edge.
//    3. Click "Connect Arduino" and pick the correct COM port.
//    4. Load your MP3 into the page (drag & drop or file picker).
//    5. Hold A, then press F  →  the page simultaneously:
//         • sends 'G' over USB-Serial  (Arduino starts its timeline)
//         • plays the MP3 from 0:00    (audio starts)
//         • runs the on-screen LED preview
//       All three fire in the same JavaScript event — frame-accurate sync.
//
//  CHANGING THE COMBO:
//    Open led_show_launcher.html and change the two key values near
//    the top of the <script> block:
//      const KEY_HOLD = 'a';   // key to hold down
//      const KEY_FIRE = 'f';   // key to tap while holding
//
//  NO WEB SERIAL? (Firefox / Safari):
//    Change WAIT_FOR_SERIAL to false below. The sketch then starts
//    immediately on power-up / reset — use the manual method instead:
//    press RESET and play the MP3 at the same moment.
//
// =============================================================================
//  WIRING — THREE PYRAMID GROUPS (4 LEDs each + 220Ω resistor per LED to GND)
//
//  Group         LED index     Digital pin     Physical meaning
//  ──────────    ──────────    ───────────     ─────────────────
//  KHUFU    (K)  0  1  2  3    2  3  4  5     Great Pyramid  (largest)
//  KHAFRE   (F)  4  5  6  7    6  7  8  9     Middle Pyramid
//  MENKAURE (M)  8  9 10 11   10 11 12 13     Smallest Pyramid
//
//  Pattern notation used in cue comments:
//    { K0,K1,K2,K3,  F0,F1,F2,F3,  M0,M1,M2,M3 }
//
// =============================================================================
//  AUDIO STRUCTURE (measured from try3.mp3)
//
//   0.0 –  3.0s  Opening narration   — high-energy speech, loud peaks at 1s, 1.5s
//   3.1 –  4.4s  Music interlude #1
//   4.5 –  7.4s  Speech continues    — gaps at 4.5s, 6.1s
//   7.5 –  8.4s  SILENCE 1.0s        — major section break
//   8.5 – 11.9s  Tonal music section — instrument swell, peak at 11s
//  12.0 – 13.4s  Brief speech phrase
//  13.9 – 15.5s  Loud speech         — peak 4097 RMS at 14s (KEY WORD)
//  16.0 – 18.1s  LOUDEST section     — 4106 → 3990 → 3354 RMS (KEY WORD)
//  18.2 – 21.9s  Quieter narration   — peak 3386 at 20s
//  22.0 – 23.9s  Music interlude #2
//  24.0 – 26.4s  Speech              — peak 3954 at 25s
//  26.5 – 28.6s  Music interlude #3  — peak 3377
//  28.7 – 29.6s  SILENCE 1.0s        — section break
//  30.0 – 34.7s  Phrase-by-phrase speech (gaps at 30.8, 31.7, 32.6, 34.8)
//  35.5 – 43.8s  Extended narration  — longest continuous speech
//  43.9 – 45.0s  SILENCE 1.2s        — section break
//  45.0 – 45.9s  Single tonal note   — ZCR=0.031, peak 3988 (musical hit)
//  46.0 – 51.9s  Speech              — steady narration
//  52.0 – 55.7s  Music section       — peaks at 53s, 54s
//  56.0 – 61.5s  Speech section
//  61.6 – 64.8s  Music section       — gaps at 63s, 64s
//  65.0 – 72.1s  Final major speech  — peak 3370 at 65s
//  72.2 – 74.6s  Music interlude
//  74.7 – 75.9s  Final speech phrase
//  76.0 – 77.1s  SILENCE 1.2s        — grand pause before finale
//  77.2 – 86.9s  Outro music         — peaks 77.5s, 79.5s (4038!), 84-85s
//
// =============================================================================

// ── Pin mapping ───────────────────────────────────────────────────────────────
const uint8_t LED_PINS[12] = {2,3,4,5,  6,7,8,9,  10,11,12,13};
const uint8_t NUM_LEDS     = 12;

// ── Serial trigger ────────────────────────────────────────────────────────────
// Set to true  → Arduino sits dark and waits for a 'G' byte over USB Serial.
//                Send it by pressing A+F on the companion web page.
// Set to false → show starts immediately on power-up / reset (old behaviour).
#define WAIT_FOR_SERIAL  true

// Internal state — do not edit
bool showReady = !WAIT_FOR_SERIAL;   // true once 'G' received (or instantly if flag is false)

// ── Animation constants ───────────────────────────────────────────────────────
// To add a new animation:
//   1. Add #define ANIM_YOURNAME N  (next number)
//   2. Add start/update functions following the patterns below
//   3. Call startYourName() from startAnimation() switch
//   4. Call updateYourName() inside loop()
#define ANIM_NONE      0   // static pattern — apply states[] immediately
#define ANIM_RIPPLE    1   // sweep Khufu→Khafre→Menkaure one LED at a time
#define ANIM_BLINK     2   // flash the ON leds in states[]
#define ANIM_PULSE     3   // breathe: Khufu on → Khafre on → Menkaure on, overlap
#define ANIM_BOUNCE    4   // ping-pong: light travels L→R then R→L across all 12
#define ANIM_EXPLODE   5   // burst from centre outward: 5,6 → 4,7 → 3,8 → 2,9…
#define ANIM_COLLAPSE  6   // implode from edges inward: 0,11 → 1,10 → 2,9…→ centre

// ── Cue structure ─────────────────────────────────────────────────────────────
struct Cue {
  uint32_t timeMs;
  uint8_t  states[12];   // { K0,K1,K2,K3, F0,F1,F2,F3, M0,M1,M2,M3 }
  uint8_t  anim;
};

// =============================================================================
//  STORY TIMELINE — precisely aligned to try3.mp3 audio analysis
//
//  Every cue timestamp was derived from measured silence gaps, energy peaks,
//  and speech/music transitions in the actual audio file.
//
//  QUICK-EDIT PATTERN GUIDE
//  ────────────────────────
//  All three on   : {1,1,1,1, 1,1,1,1, 1,1,1,1}
//  Khufu only     : {1,1,1,1, 0,0,0,0, 0,0,0,0}
//  Khafre only    : {0,0,0,0, 1,1,1,1, 0,0,0,0}
//  Menkaure only  : {0,0,0,0, 0,0,0,0, 1,1,1,1}
//  K+F            : {1,1,1,1, 1,1,1,1, 0,0,0,0}
//  F+M            : {0,0,0,0, 1,1,1,1, 1,1,1,1}
//  K+M (outer)    : {1,1,1,1, 0,0,0,0, 1,1,1,1}
//  All off        : {0,0,0,0, 0,0,0,0, 0,0,0,0}
//
// =============================================================================

const Cue cues[] PROGMEM = {

  // ══════════════════════════════════════════════════════════════════
  //  0.0 – 3.0s  OPENING NARRATION
  //  High-energy speech from the very first word.
  //  Grand entrance: all three pyramids EXPLODE outward from centre.
  // ══════════════════════════════════════════════════════════════════

  // 0:00 — All dark, then immediately EXPLODE on first word
  {     0, {1,1,1,1, 1,1,1,1, 1,1,1,1}, ANIM_EXPLODE },

  // 0:01 — First loud peak (RMS 3335) — Khufu blazes fully on, others dim
  {  1000, {1,1,1,1, 0,1,1,0, 0,0,0,0}, ANIM_NONE    },

  // 0:01.5 — Second loud peak (RMS 3659) — Khufu + Khafre outer ring
  {  1500, {1,1,1,1, 1,0,0,1, 0,0,0,0}, ANIM_NONE    },

  // 0:02.5 — Settle into all three solid while narration continues
  {  2500, {1,1,1,1, 1,1,1,1, 1,1,1,1}, ANIM_NONE    },

  // ══════════════════════════════════════════════════════════════════
  //  3.1 – 4.4s  MUSIC INTERLUDE #1
  //  First musical break — RIPPLE sweeps across all three pyramids
  // ══════════════════════════════════════════════════════════════════

  // 0:03.1 — Music starts: ripple Khufu→Khafre→Menkaure
  {  3100, {1,1,1,1, 1,1,1,1, 1,1,1,1}, ANIM_RIPPLE  },

  // ══════════════════════════════════════════════════════════════════
  //  4.5 – 7.4s  SPEECH CONTINUES
  //  Narration resumes after first musical breath.
  //  Khufu leads (Great Pyramid introduced first in most Giza narrations).
  // ══════════════════════════════════════════════════════════════════

  // 0:04.5 — Gap in audio, speech about to resume: Khufu solo
  {  4500, {1,1,1,1, 0,0,0,0, 0,0,0,0}, ANIM_NONE    },

  // 0:05.0 — Speech resumes: Khufu still glowing
  {  5000, {1,1,1,1, 0,0,0,0, 0,0,0,0}, ANIM_NONE    },

  // 0:06.1 — Gap: Khafre joins (second pyramid mentioned)
  {  6100, {1,1,1,1, 1,1,1,1, 0,0,0,0}, ANIM_NONE    },

  // ══════════════════════════════════════════════════════════════════
  //  7.5 – 8.4s  SILENCE — MAJOR SECTION BREAK #1
  //  1.0 second of measured silence. LEDs go dark to mark the break.
  // ══════════════════════════════════════════════════════════════════

  {  7500, {0,0,0,0, 0,0,0,0, 0,0,0,0}, ANIM_NONE    },

  // ══════════════════════════════════════════════════════════════════
  //  8.5 – 11.9s  TONAL MUSIC — INSTRUMENT SWELL
  //  Low zero-crossing rate = sustained musical tones.
  //  BOUNCE animation: light pings across all 12 LEDs rhythmically.
  // ══════════════════════════════════════════════════════════════════

  // 0:08.5 — Music swell begins: BOUNCE across all pyramids
  {  8500, {1,1,1,1, 1,1,1,1, 1,1,1,1}, ANIM_BOUNCE  },

  // 0:11.0 — Peak in music (RMS 3817): all solid at peak
  { 11000, {1,1,1,1, 1,1,1,1, 1,1,1,1}, ANIM_NONE    },

  // ══════════════════════════════════════════════════════════════════
  //  12.0 – 13.4s  BRIEF SPEECH PHRASE
  //  Short spoken phrase. Khufu blinks — Great Pyramid named.
  // ══════════════════════════════════════════════════════════════════

  // 0:12.0 — Brief phrase: Khufu blinks, Khafre+Menkaure quiet
  { 12000, {1,1,1,1, 0,0,0,0, 0,0,0,0}, ANIM_BLINK   },

  // ══════════════════════════════════════════════════════════════════
  //  13.9 – 15.5s  LOUD SPEECH — KEY WORD (RMS 4097 at 14s)
  //  One of the top peaks in the entire recording.
  //  Khufu EXPLODES — this is a pharaoh's name moment.
  // ══════════════════════════════════════════════════════════════════

  // 0:13.9 — Loud phrase incoming: prime all three
  { 13900, {1,1,1,1, 1,1,1,1, 1,1,1,1}, ANIM_NONE    },

  // 0:14.0 — PEAK 4097: Khufu EXPLODE from centre
  { 14000, {1,1,1,1, 0,0,0,0, 0,0,0,0}, ANIM_EXPLODE },

  // 0:15.0 — Settle: Khufu solid, others dimly supporting
  { 15000, {1,1,1,1, 0,1,1,0, 0,0,0,0}, ANIM_NONE    },

  // ══════════════════════════════════════════════════════════════════
  //  16.0 – 18.1s  LOUDEST SECTION IN ENTIRE RECORDING
  //  RMS 4106 → 3990 → 3354 — the single loudest moment.
  //  This is almost certainly "KHUFU" spoken with full emphasis.
  //  All pyramids BLAZE: KHUFU blinks furiously, others solid.
  // ══════════════════════════════════════════════════════════════════

  // 0:16.0 — PEAK 4106: Khufu BLINK (urgent), Khafre+Menkaure solid
  { 16000, {1,1,1,1, 1,1,1,1, 1,1,1,1}, ANIM_BLINK   },

  // 0:17.0 — Still loud 3354: Khufu outer pair + all of Khafre
  { 17000, {1,0,0,1, 1,1,1,1, 0,1,1,0}, ANIM_NONE    },

  // 0:17.5 — Sustaining emphasis
  { 17500, {1,1,1,1, 1,1,1,1, 1,1,1,1}, ANIM_NONE    },

  // ══════════════════════════════════════════════════════════════════
  //  18.2 – 21.9s  QUIETER NARRATION
  //  Energy drops. Narration becomes more descriptive, slower pace.
  //  PULSE: Khufu → Khafre → Menkaure breathe on in sequence.
  // ══════════════════════════════════════════════════════════════════

  // 0:18.2 — Quieter speech: PULSE across three pyramids
  { 18200, {1,1,1,1, 1,1,1,1, 1,1,1,1}, ANIM_PULSE   },

  // 0:20.0 — Mid-peak 3386: Khufu + Khafre solid, Menkaure blinks
  { 20000, {1,1,1,1, 1,1,1,1, 0,0,0,0}, ANIM_NONE    },

  // 0:21.0 — Settle all three
  { 21000, {1,1,1,1, 1,1,1,1, 1,1,1,1}, ANIM_NONE    },

  // ══════════════════════════════════════════════════════════════════
  //  22.0 – 23.9s  MUSIC INTERLUDE #2
  //  COLLAPSE: light implodes from outside edges inward.
  // ══════════════════════════════════════════════════════════════════

  { 22000, {1,1,1,1, 1,1,1,1, 1,1,1,1}, ANIM_COLLAPSE},

  // After collapse hold Khafre centre (calm tableau)
  { 23700, {0,0,0,0, 0,1,1,0, 0,0,0,0}, ANIM_NONE    },

  // ══════════════════════════════════════════════════════════════════
  //  24.0 – 26.4s  SPEECH — PEAK 3954 AT 25s
  //  Strong narration. Khafre is being described — "Khafre" keyword.
  // ══════════════════════════════════════════════════════════════════

  // 0:24.0 — Khafre introduced: EXPLODE from Khafre centre
  { 24000, {0,0,0,0, 1,1,1,1, 0,0,0,0}, ANIM_EXPLODE },

  // 0:25.0 — Peak 3954: Khafre + Khufu supporting
  { 25000, {1,0,0,1, 1,1,1,1, 0,0,0,0}, ANIM_NONE    },

  // 0:25.8 — Gap in speech: Khafre blinks
  { 25800, {0,0,0,0, 1,1,1,1, 0,0,0,0}, ANIM_BLINK   },

  // ══════════════════════════════════════════════════════════════════
  //  26.5 – 28.6s  MUSIC INTERLUDE #3 (peak 3377)
  //  RIPPLE: sweep all three, left to right.
  // ══════════════════════════════════════════════════════════════════

  { 26500, {1,1,1,1, 1,1,1,1, 1,1,1,1}, ANIM_RIPPLE  },

  // ══════════════════════════════════════════════════════════════════
  //  28.7 – 29.6s  SILENCE — MAJOR SECTION BREAK #2
  //  Another measured 1.0 second silence. All dark.
  // ══════════════════════════════════════════════════════════════════

  { 28700, {0,0,0,0, 0,0,0,0, 0,0,0,0}, ANIM_NONE    },

  // ══════════════════════════════════════════════════════════════════
  //  30.0 – 34.7s  PHRASE-BY-PHRASE SPEECH
  //  Distinct gaps at 30.8, 31.7, 32.6, 34.8s.
  //  Each phrase lights one pyramid group — call and response.
  // ══════════════════════════════════════════════════════════════════

  // Phrase 1 (30.0–30.8s): Khufu
  { 30000, {1,1,1,1, 0,0,0,0, 0,0,0,0}, ANIM_NONE    },
  // Phrase 2 (31.1–31.7s): Khafre joins
  { 31100, {1,1,1,1, 1,1,1,1, 0,0,0,0}, ANIM_NONE    },
  // Phrase 3 (32.1–32.6s): Menkaure joins — all three named
  { 32100, {1,1,1,1, 1,1,1,1, 1,1,1,1}, ANIM_NONE    },
  // Phrase 4 (32.9–34.8s): Khufu only blinks (called out specifically)
  { 32900, {1,1,1,1, 0,0,0,0, 0,0,0,0}, ANIM_BLINK   },
  // Gap 34.8s: all stop — breath
  { 34800, {0,0,0,0, 0,0,0,0, 0,0,0,0}, ANIM_NONE    },

  // ══════════════════════════════════════════════════════════════════
  //  35.5 – 43.8s  EXTENDED NARRATION (longest continuous speech)
  //  Steady storytelling. Animate the three pyramids in a flowing
  //  sequence that builds and sustains energy throughout.
  // ══════════════════════════════════════════════════════════════════

  // 35.5 — Re-entry: BOUNCE across all 12 (animated storytelling)
  { 35500, {1,1,1,1, 1,1,1,1, 1,1,1,1}, ANIM_BOUNCE  },

  // 38.0 — Mid-narration: Khufu outer + Menkaure outer (shoulders of the Giza plateau)
  { 38000, {1,0,0,1, 0,0,0,0, 1,0,0,1}, ANIM_NONE    },

  // 38.5 — Khafre takes centre (middle pyramid, balance point)
  { 38500, {0,1,1,0, 1,1,1,1, 0,1,1,0}, ANIM_NONE    },

  // 39.0 — All solid as climax of this paragraph approaches
  { 39000, {1,1,1,1, 1,1,1,1, 1,1,1,1}, ANIM_NONE    },

  // 40.5 — Gap in speech (0.5s): all dark, dramatic pause
  { 40500, {0,0,0,0, 0,0,0,0, 0,0,0,0}, ANIM_NONE    },

  // 41.0 — Speech resumes: EXPLODE back on
  { 41000, {1,1,1,1, 1,1,1,1, 1,1,1,1}, ANIM_EXPLODE },

  // 42.7 — Short gap approaching: fade to Khafre only
  { 42700, {0,0,0,0, 1,1,1,1, 0,0,0,0}, ANIM_NONE    },

  // ══════════════════════════════════════════════════════════════════
  //  43.9 – 45.0s  SILENCE 1.2s — SECTION BREAK #3
  //  Longest silence yet. Full blackout — maximum dramatic pause.
  // ══════════════════════════════════════════════════════════════════

  { 43900, {0,0,0,0, 0,0,0,0, 0,0,0,0}, ANIM_NONE    },

  // ══════════════════════════════════════════════════════════════════
  //  45.0 – 45.9s  SINGLE TONAL HIT (ZCR=0.031, RMS 3988)
  //  The purest musical note in the whole file — instrument hit.
  //  COLLAPSE inward to a single central flash, then hold.
  // ══════════════════════════════════════════════════════════════════

  { 45000, {1,1,1,1, 1,1,1,1, 1,1,1,1}, ANIM_COLLAPSE},
  { 45700, {0,0,0,0, 0,1,1,0, 0,0,0,0}, ANIM_NONE    },  // single Khafre centre note

  // ══════════════════════════════════════════════════════════════════
  //  46.0 – 51.9s  SPEECH — MENKAURE SECTION
  //  Steady narration. Now describing Menkaure — third pyramid.
  //  Menkaure blinks, then all three are introduced together.
  // ══════════════════════════════════════════════════════════════════

  // 0:46.0 — Menkaure named: EXPLODE from Menkaure
  { 46000, {0,0,0,0, 0,0,0,0, 1,1,1,1}, ANIM_EXPLODE },

  // 0:46.5 — Menkaure steady
  { 46500, {0,0,0,0, 0,0,0,0, 1,1,1,1}, ANIM_NONE    },

  // 0:47.0 — Khufu + Menkaure (outer pyramids flanking Khafre)
  { 47000, {1,1,1,1, 0,0,0,0, 1,1,1,1}, ANIM_NONE    },

  // 0:47.8 — Gap: brief dark
  { 47800, {0,0,0,0, 0,0,0,0, 0,0,0,0}, ANIM_NONE    },

  // 0:48.2 — All three reunited: RIPPLE
  { 48200, {1,1,1,1, 1,1,1,1, 1,1,1,1}, ANIM_RIPPLE  },

  // 0:49.0 — Steady narration: all three solid
  { 49500, {1,1,1,1, 1,1,1,1, 1,1,1,1}, ANIM_NONE    },

  // 0:51.0 — Final phrase of section: Menkaure blinks alone
  { 51000, {0,0,0,0, 0,0,0,0, 1,1,1,1}, ANIM_BLINK   },

  // ══════════════════════════════════════════════════════════════════
  //  52.0 – 55.7s  MUSIC SECTION (peaks at 53s, 54s)
  //  Rich musical passage. BOUNCE across all three pyramids.
  // ══════════════════════════════════════════════════════════════════

  // 0:52.0 — Silence then music: brief dark, then BOUNCE
  { 52000, {0,0,0,0, 0,0,0,0, 0,0,0,0}, ANIM_NONE    },
  { 52500, {1,1,1,1, 1,1,1,1, 1,1,1,1}, ANIM_BOUNCE  },

  // 0:53.0 — Peak 3229: all solid during music climax
  { 53000, {1,1,1,1, 1,1,1,1, 1,1,1,1}, ANIM_NONE    },

  // 0:54.0 — Peak 3371: Khufu + Menkaure (outer pair)
  { 54000, {1,1,1,1, 0,0,0,0, 1,1,1,1}, ANIM_NONE    },

  // 0:55.0 — Music fading: just Khafre inner pair
  { 55000, {0,0,0,0, 0,1,1,0, 0,0,0,0}, ANIM_NONE    },

  // 0:55.8 — Gap: all dark
  { 55800, {0,0,0,0, 0,0,0,0, 0,0,0,0}, ANIM_NONE    },

  // ══════════════════════════════════════════════════════════════════
  //  56.0 – 61.5s  SPEECH SECTION
  //  Narration continues. Steady mid-energy.
  //  Pyramids light in a rolling pattern — slow PULSE.
  // ══════════════════════════════════════════════════════════════════

  // 0:56.5 — Speech opens: PULSE (breathe on pyramid by pyramid)
  { 56500, {1,1,1,1, 1,1,1,1, 1,1,1,1}, ANIM_PULSE   },

  // 0:58.0 — Khufu + Khafre (largest two)
  { 58000, {1,1,1,1, 1,1,1,1, 0,0,0,0}, ANIM_NONE    },

  // 0:59.3 — Gap, then speech again: all three
  { 59600, {1,1,1,1, 1,1,1,1, 1,1,1,1}, ANIM_NONE    },

  // 1:01.0 — Building to music: COLLAPSE inward
  { 61000, {1,1,1,1, 1,1,1,1, 1,1,1,1}, ANIM_COLLAPSE},

  // ══════════════════════════════════════════════════════════════════
  //  61.6 – 64.8s  MUSIC SECTION (gaps at 63s, 64s)
  //  Sparse musical passage with notable gaps.
  //  Mirror the gaps in the music: LEDs wink on/off.
  // ══════════════════════════════════════════════════════════════════

  // 1:01.6 — Music gap: dark
  { 61600, {0,0,0,0, 0,0,0,0, 0,0,0,0}, ANIM_NONE    },
  // 1:02.0 — Music note: Khufu flashes
  { 62000, {1,1,1,1, 0,0,0,0, 0,0,0,0}, ANIM_NONE    },
  // 1:03.0 — Measured gap in music: all dark
  { 63000, {0,0,0,0, 0,0,0,0, 0,0,0,0}, ANIM_NONE    },
  // 1:03.5 — Returns: Khafre alone
  { 63500, {0,0,0,0, 1,1,1,1, 0,0,0,0}, ANIM_NONE    },
  // 1:04.0 — Measured gap: dark
  { 64000, {0,0,0,0, 0,0,0,0, 0,0,0,0}, ANIM_NONE    },
  // 1:04.5 — Final pause before speech: Menkaure alone
  { 64500, {0,0,0,0, 0,0,0,0, 1,1,1,1}, ANIM_NONE    },

  // ══════════════════════════════════════════════════════════════════
  //  65.0 – 72.1s  FINAL MAJOR SPEECH SECTION
  //  Peak 3370 at 65s, steady through 71s.
  //  This is the emotional climax of the narration.
  //  EXPLODE → hold → BOUNCE as energy builds to finale.
  // ══════════════════════════════════════════════════════════════════

  // 1:05.0 — Peak 3370: ALL THREE EXPLODE together — climactic entry
  { 65000, {1,1,1,1, 1,1,1,1, 1,1,1,1}, ANIM_EXPLODE },

  // 1:06.0 — All solid as narration hits stride
  { 66000, {1,1,1,1, 1,1,1,1, 1,1,1,1}, ANIM_NONE    },

  // 1:06.9 — Gap: outer LEDs only (silhouette of all three pyramids)
  { 66900, {1,0,0,1, 1,0,0,1, 1,0,0,1}, ANIM_NONE    },

  // 1:07.5 — Inner LEDs join (pyramids fill in)
  { 67500, {0,1,1,0, 0,1,1,0, 0,1,1,0}, ANIM_NONE    },

  // 1:08.1 — All solid
  { 68100, {1,1,1,1, 1,1,1,1, 1,1,1,1}, ANIM_NONE    },

  // 1:09.0 — BOUNCE: energy builds toward peak at 70.5s
  { 69000, {1,1,1,1, 1,1,1,1, 1,1,1,1}, ANIM_BOUNCE  },

  // 1:10.5 — Peak 3333 at 70.5s: pause BOUNCE, go solid
  { 70500, {1,1,1,1, 1,1,1,1, 1,1,1,1}, ANIM_NONE    },

  // 1:11.0 — Khufu strobe (emphasis) approaching music
  { 71000, {1,1,1,1, 0,1,1,0, 0,0,0,0}, ANIM_NONE    },

  // ══════════════════════════════════════════════════════════════════
  //  72.2 – 74.6s  MUSIC INTERLUDE (before final phrase)
  //  COLLAPSE: everything narrows to a pinpoint.
  // ══════════════════════════════════════════════════════════════════

  { 72200, {1,1,1,1, 1,1,1,1, 1,1,1,1}, ANIM_COLLAPSE},
  { 73500, {0,0,0,0, 0,1,1,0, 0,0,0,0}, ANIM_NONE    },  // just Khafre centre

  // ══════════════════════════════════════════════════════════════════
  //  74.7 – 75.9s  FINAL SPEECH PHRASE
  //  Last words of the narration. Khufu blinks one last time.
  // ══════════════════════════════════════════════════════════════════

  { 74700, {1,1,1,1, 0,0,0,0, 0,0,0,0}, ANIM_BLINK   },

  // ══════════════════════════════════════════════════════════════════
  //  76.0 – 77.1s  SILENCE 1.2s — GRAND PAUSE BEFORE FINALE
  //  The longest silence at the emotional peak of the track.
  //  Complete blackout for maximum drama.
  // ══════════════════════════════════════════════════════════════════

  { 76000, {0,0,0,0, 0,0,0,0, 0,0,0,0}, ANIM_NONE    },

  // ══════════════════════════════════════════════════════════════════
  //  77.2 – 86.9s  OUTRO MUSIC — GRAND FINALE
  //  Biggest music in the track: peaks at 77.5s, 79.5s (4038!), 84-85s.
  //  RIPPLE → all solid → silence → EXPLODE → RIPPLE → final glow
  // ══════════════════════════════════════════════════════════════════

  // 1:17.2 — Outro begins: RIPPLE Khufu→Khafre→Menkaure majestically
  { 77200, {1,1,1,1, 1,1,1,1, 1,1,1,1}, ANIM_RIPPLE  },

  // 1:17.5 — Peak 3828: hold solid after ripple settles
  { 77800, {1,1,1,1, 1,1,1,1, 1,1,1,1}, ANIM_NONE    },

  // 1:18.1 — Silence within outro (gap 1.2s): blackout
  { 78100, {0,0,0,0, 0,0,0,0, 0,0,0,0}, ANIM_NONE    },

  // 1:19.3 — BIGGEST PEAK IN FILE: 4038 RMS — EXPLODE everything
  { 79300, {1,1,1,1, 1,1,1,1, 1,1,1,1}, ANIM_EXPLODE },

  // 1:19.5 — Sustain peak: all three blazing solid
  { 79800, {1,1,1,1, 1,1,1,1, 1,1,1,1}, ANIM_NONE    },

  // 1:20.5 — Second RIPPLE across all (music sustains)
  { 80500, {1,1,1,1, 1,1,1,1, 1,1,1,1}, ANIM_RIPPLE  },

  // 1:21.0 — Khufu + Khafre (outer two, Menkaure dims)
  { 81000, {1,1,1,1, 1,1,1,1, 0,0,0,0}, ANIM_NONE    },

  // 1:21.9 — Silence gap 0.9s: dark
  { 81900, {0,0,0,0, 0,0,0,0, 0,0,0,0}, ANIM_NONE    },

  // 1:22.8 — Returns: BOUNCE through the silence aftermath
  { 82800, {1,1,1,1, 1,1,1,1, 1,1,1,1}, ANIM_BOUNCE  },

  // 1:23.5 — Gap 0.5s: pause to breathe
  { 83500, {0,1,1,0, 0,1,1,0, 0,1,1,0}, ANIM_NONE    },  // inner LEDs glow

  // ══════════════════════════════════════════════════════════════════
  //  84.0 – 86.9s  FINAL NARRATION OVER MUSIC — CLOSING WORDS
  //  RMS 3318 → 3504 → 3121. Strong, dignified final words.
  //  Build from Menkaure → Khafre → Khufu, then slow fade.
  // ══════════════════════════════════════════════════════════════════

  // 1:24.0 — Final speech begins: Menkaure lights (smallest, mentioned last)
  { 84000, {0,0,0,0, 0,0,0,0, 1,1,1,1}, ANIM_NONE    },

  // 1:24.5 — Khafre joins
  { 84500, {0,0,0,0, 1,1,1,1, 1,1,1,1}, ANIM_NONE    },

  // 1:25.0 — Peak 3504: Khufu completes the trio — all three stand proud
  { 85000, {1,1,1,1, 1,1,1,1, 1,1,1,1}, ANIM_NONE    },

  // 1:25.5 — COLLAPSE to Khafre centre — the pyramids fade to a single light
  { 85500, {1,1,1,1, 1,1,1,1, 1,1,1,1}, ANIM_COLLAPSE},

  // 1:26.0 — Final breath: just Khafre centre pair
  { 86000, {0,0,0,0, 0,1,1,0, 0,0,0,0}, ANIM_NONE    },

  // 1:26.5 — Darkness. Show complete.
  { 86500, {0,0,0,0, 0,0,0,0, 0,0,0,0}, ANIM_NONE    },
};

const uint16_t NUM_CUES = sizeof(cues) / sizeof(cues[0]);

// =============================================================================
//  ANIMATION TIMING CONSTANTS
// =============================================================================
#define RIPPLE_STEP_MS    70UL   // ms between each LED in ripple sweep
#define BLINK_PERIOD_MS  220UL   // ms per blink half-cycle
#define BOUNCE_STEP_MS    60UL   // ms per LED hop in bounce
#define PULSE_STEP_MS    200UL   // ms per group in pulse wave
#define EXPLODE_STEP_MS   55UL   // ms per ring in explode/collapse

// =============================================================================
//  ANIMATION STATE STRUCTS  (all non-blocking — zero delay() calls)
// =============================================================================

struct RippleState  { bool active; uint8_t step; uint32_t lastMs; };
struct BlinkState   { bool active; bool phase; uint32_t lastMs; uint8_t mask[12]; };
struct BounceState  { bool active; int8_t pos; int8_t dir; uint32_t lastMs; };
struct PulseState   { bool active; uint8_t group; uint32_t lastMs; };  // group 0=K,1=F,2=M
struct ExplodeState { bool active; uint8_t ring; bool inward; uint32_t lastMs; };

RippleState  ripple  = {false,0,0};
BlinkState   blink   = {false,true,0,{0}};
BounceState  bounce  = {false,0,1,0};
PulseState   pulse   = {false,0,0};
ExplodeState explode = {false,0,false,0};

// =============================================================================
//  RUNTIME VARIABLES
// =============================================================================
uint16_t currentCue    = 0;
uint32_t showStartMs   = 0;
uint32_t activeCueTimeMs;
uint8_t  activeCueStates[12];
uint8_t  activeCueAnim;

// =============================================================================
//  HELPERS
// =============================================================================
void loadCue(uint16_t idx) {
  activeCueTimeMs = pgm_read_dword(&cues[idx].timeMs);
  activeCueAnim   = pgm_read_byte(&cues[idx].anim);
  for (uint8_t i = 0; i < NUM_LEDS; i++)
    activeCueStates[i] = pgm_read_byte(&cues[idx].states[i]);
}

void setLed(uint8_t idx, uint8_t on) {
  digitalWrite(LED_PINS[idx], on ? HIGH : LOW);
}

void allOff() {
  for (uint8_t i = 0; i < NUM_LEDS; i++) setLed(i, 0);
}

void applyPattern(const uint8_t s[]) {
  for (uint8_t i = 0; i < NUM_LEDS; i++) setLed(i, s[i]);
}

void cancelAll() {
  ripple.active = false;
  blink.active  = false;
  bounce.active = false;
  pulse.active  = false;
  explode.active= false;
}

// =============================================================================
//  ANIMATION: RIPPLE  (Khufu → Khafre → Menkaure, one LED at a time)
// =============================================================================
void startRipple() {
  cancelAll(); allOff();
  ripple = {true, 0, millis()};
}
void updateRipple() {
  if (!ripple.active) return;
  uint32_t now = millis();
  if (now - ripple.lastMs >= RIPPLE_STEP_MS) {
    if (ripple.step < NUM_LEDS) {
      setLed(ripple.step++, 1);
      ripple.lastMs = now;
    } else {
      ripple.active = false;
      applyPattern(activeCueStates);
    }
  }
}

// =============================================================================
//  ANIMATION: BLINK  (flash ON leds in states[])
// =============================================================================
void startBlink(const uint8_t s[]) {
  cancelAll();
  for (uint8_t i = 0; i < NUM_LEDS; i++) blink.mask[i] = s[i];
  blink = {true, true, millis(), {0}};
  for (uint8_t i = 0; i < NUM_LEDS; i++) blink.mask[i] = s[i];
  applyPattern(s);
}
void updateBlink() {
  if (!blink.active) return;
  uint32_t now = millis();
  if (now - blink.lastMs >= BLINK_PERIOD_MS) {
    blink.phase  = !blink.phase;
    blink.lastMs = now;
    for (uint8_t i = 0; i < NUM_LEDS; i++)
      setLed(i, blink.phase && blink.mask[i] ? 1 : 0);
  }
}

// =============================================================================
//  ANIMATION: BOUNCE  (single lit LED ping-pongs across all 12)
//  Great for music sections — feels rhythmic.
// =============================================================================
void startBounce() {
  cancelAll(); allOff();
  bounce = {true, 0, 1, millis()};
  setLed(0, 1);
}
void updateBounce() {
  if (!bounce.active) return;
  uint32_t now = millis();
  if (now - bounce.lastMs >= BOUNCE_STEP_MS) {
    setLed(bounce.pos, 0);          // erase old position
    bounce.pos += bounce.dir;
    if (bounce.pos >= NUM_LEDS-1) { bounce.pos = NUM_LEDS-1; bounce.dir = -1; }
    if (bounce.pos <= 0)           { bounce.pos = 0;          bounce.dir =  1; }
    setLed(bounce.pos, 1);
    bounce.lastMs = now;
  }
}

// =============================================================================
//  ANIMATION: PULSE  (sequential group fade-on: Khufu → Khafre → Menkaure)
//  Each pyramid group lights in turn, creating a slow wave of light.
//  When group 2 (Menkaure) finishes, it loops back to Khufu.
// =============================================================================
void startPulse() {
  cancelAll(); allOff();
  pulse = {true, 0, millis()};
  // Light Khufu first (group 0 = indices 0-3)
  for (uint8_t i = 0; i < 4; i++) setLed(i, 1);
}
void updatePulse() {
  if (!pulse.active) return;
  uint32_t now = millis();
  if (now - pulse.lastMs >= PULSE_STEP_MS) {
    pulse.group = (pulse.group + 1) % 3;
    pulse.lastMs = now;
    // Light only the active group
    for (uint8_t g = 0; g < 3; g++) {
      uint8_t on = (g == pulse.group) ? 1 : 0;
      for (uint8_t i = g*4; i < g*4+4; i++) setLed(i, on);
    }
  }
}

// =============================================================================
//  ANIMATION: EXPLODE  (burst from centre outward)
//  Rings expand: centre pair → next pair out → next → edges.
//  Led order for outward rings from position 5,6 (Khafre centre):
//    Ring 0: idx 5,6  (Khafre F1,F2)
//    Ring 1: idx 4,7  (Khafre F0,F3)
//    Ring 2: idx 3,8  (Khufu K3, Menkaure M0)
//    Ring 3: idx 2,9  (Khufu K2, Menkaure M1)
//    Ring 4: idx 1,10 (Khufu K1, Menkaure M2)
//    Ring 5: idx 0,11 (Khufu K0, Menkaure M3)
//
//  COLLAPSE is the reverse (inward): ring 5→0.
// =============================================================================

// Ring definition: pairs of LED indices from centre outward
const uint8_t RINGS[6][2] = {{5,6},{4,7},{3,8},{2,9},{1,10},{0,11}};

void startExplode() {
  cancelAll(); allOff();
  explode = {true, 0, false, millis()};
  setLed(RINGS[0][0], 1); setLed(RINGS[0][1], 1);  // light centre ring
}
void startCollapse() {
  cancelAll();
  // Start with all on, then extinguish from outside in
  for (uint8_t i = 0; i < NUM_LEDS; i++) setLed(i, 1);
  explode = {true, 0, true, millis()};
}
void updateExplode() {
  if (!explode.active) return;
  uint32_t now = millis();
  if (now - explode.lastMs >= EXPLODE_STEP_MS) {
    explode.ring++;
    explode.lastMs = now;
    if (explode.ring >= 6) {
      explode.active = false;
      applyPattern(activeCueStates);
      return;
    }
    if (!explode.inward) {
      // EXPLODE: light next outer ring
      setLed(RINGS[explode.ring][0], 1);
      setLed(RINGS[explode.ring][1], 1);
    } else {
      // COLLAPSE: turn off next outer ring (working from outside in)
      uint8_t outerRing = 5 - explode.ring;
      setLed(RINGS[outerRing][0], 0);
      setLed(RINGS[outerRing][1], 0);
    }
  }
}

// =============================================================================
//  DISPATCH
// =============================================================================
void startAnimation(uint8_t anim, const uint8_t s[]) {
  switch (anim) {
    case ANIM_RIPPLE:   startRipple();   break;
    case ANIM_BLINK:    startBlink(s);   break;
    case ANIM_BOUNCE:   startBounce();   break;
    case ANIM_PULSE:    startPulse();    break;
    case ANIM_EXPLODE:  startExplode();  break;
    case ANIM_COLLAPSE: startCollapse(); break;
    default:
      cancelAll();
      applyPattern(s);
      break;
  }
}

// =============================================================================
//  SETUP
// =============================================================================
void setup() {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    setLed(i, 0);
  }

#if WAIT_FOR_SERIAL
  // Open Serial at 9600 baud and wait for the 'G' byte from the web page.
  // All LEDs stay off and the timeline does NOT start until 'G' arrives.
  // The web page sends 'G' the moment A+F is pressed, at the same instant
  // it starts the MP3 — so both stay perfectly in sync.
  Serial.begin(9600);
  // Blink the very first LED slowly so you know the Arduino is alive
  // and waiting. (The blink stops the moment 'G' is received.)
  bool waitBlink = false;
  uint32_t waitLast = millis();
  while (!Serial.available()) {
    uint32_t now = millis();
    if (now - waitLast >= 500) {
      waitBlink = !waitBlink;
      setLed(0, waitBlink ? 1 : 0);   // LED on pin 2 blinks every 500 ms
      waitLast = now;
    }
  }
  // Drain all bytes (there may be more than one)
  while (Serial.available()) Serial.read();
  setLed(0, 0);   // make sure the blink LED is off before show starts
  showReady = true;
#else
  // No Serial wait — start immediately (useful if Web Serial is unavailable)
  Serial.begin(9600);   // still open so you can send 'R' to reset mid-show
#endif

  showStartMs = millis();   // t=0 is the moment 'G' was received
  loadCue(0);
  startAnimation(activeCueAnim, activeCueStates);
}

// =============================================================================
//  LOOP  (100% non-blocking)
// =============================================================================
void loop() {
  // ── Serial commands (always checked, even mid-show) ──────────────────────
  //   'G'  →  restart the show from cue 0  (same as pressing A+F again)
  //   'R'  →  same as 'G'  (alias, for convenience)
  if (Serial.available()) {
    char cmd = Serial.read();
    if (cmd == 'G' || cmd == 'g' || cmd == 'R' || cmd == 'r') {
      cancelAll();
      allOff();
      currentCue  = 0;
      showStartMs = millis();
      showReady   = true;
      loadCue(0);
      startAnimation(activeCueAnim, activeCueStates);
    }
  }

  // ── Hold here until the go signal has been received ──────────────────────
  if (!showReady) return;

  uint32_t elapsed = millis() - showStartMs;

  // Advance cue when timestamp arrives
  if (currentCue + 1 < NUM_CUES) {
    if (elapsed >= pgm_read_dword(&cues[currentCue + 1].timeMs)) {
      currentCue++;
      loadCue(currentCue);
      startAnimation(activeCueAnim, activeCueStates);
    }
  }

  // Service all animations
  updateRipple();
  updateBlink();
  updateBounce();
  updatePulse();
  updateExplode();
}

// =============================================================================
//  ADDING NEW CUES — QUICK REFERENCE
// =============================================================================
//
//  1. Find the timestamp from your audio editor (ms from start).
//  2. Choose a pattern:
//       Khufu only     : {1,1,1,1, 0,0,0,0, 0,0,0,0}
//       Khafre only    : {0,0,0,0, 1,1,1,1, 0,0,0,0}
//       Menkaure only  : {0,0,0,0, 0,0,0,0, 1,1,1,1}
//       All three      : {1,1,1,1, 1,1,1,1, 1,1,1,1}
//  3. Choose an animation:
//       ANIM_NONE      – instant static
//       ANIM_RIPPLE    – sweep left to right (good for music)
//       ANIM_BLINK     – flash (good for keyword emphasis)
//       ANIM_BOUNCE    – ping-pong LED (good for music passages)
//       ANIM_PULSE     – group-by-group slow wave (good for narration)
//       ANIM_EXPLODE   – burst from centre (good for loud peaks)
//       ANIM_COLLAPSE  – implode to centre (good for endings/pauses)
//  4. Insert the line in ascending time order inside cues[].
//
//  SCALING FOR A DIFFERENT MP3 LENGTH:
//    new_ms = old_ms * YOUR_TRACK_MS / 86961
//
// =============================================================================
//  WEB TRIGGER — KEYBOARD SHORTCUT  (A + F)
// =============================================================================
//
//  HOW IT WORKS END-TO-END
//  ───────────────────────
//  1. Arduino powers on → all LEDs off → LED on pin 2 blinks slowly.
//     (The sketch is sitting inside Serial.available() waiting for 'G'.)
//
//  2. User opens the website in Chrome or Edge.
//     Page connects to the Arduino's COM port via Web Serial API.
//
//  3. User holds A, then taps F.
//     In the SAME JavaScript event the page does three things at once:
//       a. Sends the byte 'G' over USB Serial  → Arduino receives it,
//          stamps showStartMs = millis(), and fires cue 0.
//       b. Calls audioElement.play()           → MP3 starts from 0:00.
//       c. Starts the on-screen LED preview    → visual sync indicator.
//
//  4. Show runs. Press A+F again at any time to restart from cue 0.
//
//  JAVASCRIPT SNIPPET (what the web page does)
//  ────────────────────────────────────────────
//    const KEY_HOLD = 'a';          // ← change here to remap the combo
//    const KEY_FIRE = 'f';
//
//    let holdDown = false;
//    let port, writer;
//
//    async function connectArduino() {
//      port   = await navigator.serial.requestPort();
//      await port.open({ baudRate: 9600 });
//      writer = port.writable.getWriter();
//    }
//
//    async function triggerShow() {
//      // a. send go byte
//      if (writer) await writer.write(new Uint8Array([0x47])); // 'G'
//      // b. start audio
//      audio.currentTime = 0;
//      audio.play();
//      // c. start on-screen preview
//      startPreview();
//    }
//
//    document.addEventListener('keydown', e => {
//      if (e.key.toLowerCase() === KEY_HOLD) holdDown = true;
//      if (e.key.toLowerCase() === KEY_FIRE && holdDown) triggerShow();
//    });
//    document.addEventListener('keyup', e => {
//      if (e.key.toLowerCase() === KEY_HOLD) holdDown = false;
//    });
//
//  CHANGING THE COMBO
//  ───────────────────
//  Edit KEY_HOLD and KEY_FIRE in the JS above.
//  The Arduino side doesn't care — it only looks for the 'G' byte,
//  whatever keyboard combo sends it.
//
//  NO WEB SERIAL? (Firefox / Safari)
//  ───────────────────────────────────
//  Set  #define WAIT_FOR_SERIAL  false  at the top of this file.
//  The show starts on power-up / reset instead.
//  Workaround: press RESET and tap Play on the MP3 at the same moment.
//
//  MID-SHOW RESTART
//  ─────────────────
//  Sending 'G' or 'R' at any time (A+F again, or via Serial Monitor)
//  resets the timeline to cue 0, restamps the clock, and restarts.
//
// =============================================================================

