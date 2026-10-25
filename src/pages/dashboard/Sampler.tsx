import { Download, FileCode2, Layers } from "lucide-react";

import { Button } from "@/components/ui/button";
import {
  PLAY_MODES,
  SAMPLER_DOWNLOAD_FILES,
  SAMPLER_FACTS,
  SAMPLER_ROUTING,
  SAMPLER_ZIP,
  SHARED_CONTROLS,
  SLOT_KNOBS,
  samplerDownloadUrl,
} from "@/lib/sampler";

const STEPS = [
  {
    title: "Unzip next to the other two",
    body: "So the folder lands at koola10-wavetable-synth\\koola10-sampler\\. It is a separate plugin with its own CMake project; nothing in the synth or the drum machine moves.",
  },
  {
    title: "Open the sampler's plugin-juce folder",
    body: "In Visual Studio: File → Open → Folder, and pick koola10-sampler\\plugin-juce (the one containing CMakeLists.txt).",
  },
  {
    title: "Let CMake configure",
    body: "Visual Studio starts on its own. The first configure downloads JUCE, so give it a few minutes and a connection.",
  },
  {
    title: "Build All",
    body: "Build → Build All, or Ctrl+Shift+B. Use Release for making music; Debug is only for stepping through code.",
  },
  {
    title: "Load a sound and press KEYS",
    body: "The VST3 is copied to C:\\Program Files\\Common Files\\VST3\\Koola10 Sampler.vst3. All eight slots start empty on purpose: this plugin plays your samples, it does not ship a library.",
  },
];

const LISTENING = [
  {
    title: "It tracks pitch cleanly across two octaves",
    body: "Load one of your own one-shots, set KEYS on that slot, and play it across two octaves of the piano roll. It should still sound like your sound, not a chipmunk. If the top of the range goes glassy, that is the sample itself running out of top end.",
  },
  {
    title: "A trimmed loop has no click at the seam",
    body: "Set Start and End by ear, switch the slot to Loop and hold a note. If there is still a tick, nudge End a percent or two: the click is the sample restarting at the wrong phase.",
  },
  {
    title: "Every knob does what its label says",
    body: "Turn each one on its own and listen. Every readout is a percentage except FILTER CUTOFF, which stays in kHz on purpose.",
  },
  {
    title: "All eight slots come back with the project",
    body: "Close the project and reopen it. Paths, play modes and knobs should all be exactly where you left them. Move a file away and that slot goes silent rather than crashing.",
  },
];

const NOT_IN_V1 = [
  "No auto-slicing or chopping — trimming is Start and End",
  "No time-stretch — the pitch follows the note",
  "No bundled sample library",
  "No recording or resampling into the plugin",
  "No sequencing — the drum machine already does that",
  "No vocal processing, no AI anything",
];

export default function Sampler() {
  return (
    <div className="space-y-14">
      <header className="border-b border-border pb-8">
        <p className="label-mono">Koola10 Sampler · v1</p>
        <h1 className="mt-3 text-2xl font-bold tracking-tight">
          Eight slots, your sounds, across the whole keyboard.
        </h1>
        <p className="mt-3 max-w-2xl text-sm leading-6 text-muted-foreground">
          WAV and MP3, sixteen voices, and a trim on every slot so a hit lands on
          the beat instead of after it. Same panel language as the synth and the
          drum machine: percentages everywhere except the filter cutoff, which
          stays in kHz.
        </p>

        <div className="mt-6 flex flex-wrap items-center gap-3">
          <Button asChild className="rounded-none">
            <a href={SAMPLER_ZIP} download>
              <Layers className="size-4" />
              Download v1 (.zip)
            </a>
          </Button>
          <Button asChild variant="outline" className="rounded-none">
            <a href={samplerDownloadUrl("README.md")}>
              <FileCode2 className="size-4" />
              Read the build guide
            </a>
          </Button>
          <span className="label-mono">Standalone + VST3 · VS2022</span>
        </div>
      </header>

      <section>
        <h2 className="label-mono">What it is</h2>
        <div className="mt-4 grid gap-px bg-border sm:grid-cols-2">
          {SAMPLER_FACTS.map((fact) => (
            <div key={fact.title} className="bg-background p-6">
              <h3 className="text-sm font-bold tracking-tight">{fact.title}</h3>
              <p className="mt-3 text-sm leading-6 text-muted-foreground">{fact.body}</p>
            </div>
          ))}
        </div>
      </section>

      <section>
        <h2 className="label-mono">The panel</h2>
        <div className="mt-4 border border-border bg-[#0b0b0c] p-5">
          <div className="flex items-baseline justify-between gap-4 border-b border-white/10 pb-3">
            <p className="text-xs font-bold tracking-[0.2em] text-[#ececec]">KOOLA10 SAMPLER</p>
            <p className="font-mono text-[10px] tracking-[0.2em] text-[#808088]">
              SAMPLE PLAYER · V1 · WAV + MP3
            </p>
          </div>

          <div className="mt-4 grid gap-3 sm:grid-cols-2">
            {Array.from({ length: 8 }).map((_, index) => (
              <div
                key={index}
                className={`border p-3 ${
                  index === 0 ? "border-[#eaeaea]" : "border-white/10"
                }`}
              >
                <div className="flex items-baseline justify-between gap-3">
                  <p className="text-[10px] font-bold tracking-[0.16em] text-[#ececec]">
                    SLOT {index + 1}
                  </p>
                  <p className="font-mono text-[9px] tracking-[0.12em] text-[#808088]">
                    {index === 0 ? "diner_kick.wav" : "EMPTY"}
                  </p>
                </div>

                <div className="mt-2 flex h-6 items-center gap-[2px]">
                  {Array.from({ length: 34 }).map((__, bar) => {
                    const height =
                      index === 0 ? 4 + Math.abs(Math.sin((bar + index) * 0.9)) * 18 : 1;
                    return (
                      <span
                        key={bar}
                        className="w-[3px] bg-[#ececec]"
                        style={{ height: `${height}px`, opacity: index === 0 ? 0.72 : 0.14 }}
                      />
                    );
                  })}
                </div>

                <div className="mt-2 grid grid-cols-6 gap-1">
                  {SLOT_KNOBS.map((knob) => (
                    <div key={knob.suffix} className="text-center">
                      <div className="mx-auto size-5 rounded-full border border-white/20" />
                      <p className="mt-1 text-[7px] tracking-[0.12em] text-[#808088]">
                        {knob.label.toUpperCase()}
                      </p>
                    </div>
                  ))}
                </div>

                <div className="mt-2 flex items-center justify-between gap-2 border-t border-white/10 pt-2">
                  <p className="font-mono text-[9px] text-[#808088]">
                    {index === 0 ? "LOOP" : "ONE-SHOT"}
                  </p>
                  <p className="font-mono text-[9px] text-[#eaeaea]">
                    {index === 0 ? "KEYS" : "LOAD"}
                  </p>
                </div>
              </div>
            ))}
          </div>

          <div className="mt-4 grid gap-3 border-t border-white/10 pt-4 sm:grid-cols-3">
            {SHARED_CONTROLS.map((control) => (
              <div key={control.id} className="text-center">
                <div className="mx-auto size-8 rounded-full border border-white/20" />
                <p className="mt-2 text-[8px] tracking-[0.16em] text-[#808088]">
                  {control.label.toUpperCase()}
                </p>
                <p className="font-mono text-[9px] text-[#ececec]">{control.defaultValue}</p>
              </div>
            ))}
          </div>
        </div>
        <p className="mt-3 text-xs leading-6 text-muted-foreground">
          Two columns of four slots, the shared filter and master level on the
          right, and the card with the brighter frame is the one the piano roll
          is playing.
        </p>
      </section>

      <section>
        <h2 className="label-mono">Six knobs on every slot</h2>
        <div className="mt-4 grid gap-px bg-border sm:grid-cols-2 lg:grid-cols-3">
          {SLOT_KNOBS.map((knob) => (
            <div key={knob.suffix} className="bg-background p-6">
              <div className="flex items-baseline justify-between gap-3">
                <h3 className="text-sm font-bold tracking-tight">{knob.label}</h3>
                <span className="font-mono text-[10px] text-muted-foreground">
                  {knob.suffix}
                </span>
              </div>
              <p className="mt-1 text-[11px] text-muted-foreground">
                {knob.range} · default {knob.defaultValue}
              </p>
              <p className="mt-3 text-sm leading-6 text-muted-foreground">{knob.whatItDoes}</p>
              <p className="mt-3 border-t border-border pt-3 text-sm leading-6 text-muted-foreground">
                {knob.whatYouHear}
              </p>
            </div>
          ))}
        </div>
      </section>

      <section>
        <h2 className="label-mono">One-shot or loop</h2>
        <div className="mt-4 grid gap-px bg-border sm:grid-cols-2">
          {PLAY_MODES.map((mode) => (
            <div key={mode.name} className="bg-background p-6">
              <h3 className="text-sm font-bold tracking-tight">{mode.name}</h3>
              <p className="mt-3 text-sm leading-6 text-muted-foreground">{mode.whatItDoes}</p>
              <p className="mt-3 border-t border-border pt-3 text-sm leading-6 text-muted-foreground">
                {mode.whatYouHear}
              </p>
            </div>
          ))}
        </div>
        <p className="mt-4 text-sm leading-6 text-muted-foreground">
          To make a loop that does not click, set Start and End by ear on the
          waveform until the bright part begins and ends on the same point of the
          file, then hold a note. A percent or two either way is usually the
          difference.
        </p>
      </section>

      <section>
        <h2 className="label-mono">Shared controls</h2>
        <div className="mt-4 divide-y divide-border border-y border-border">
          {SHARED_CONTROLS.map((control) => (
            <div key={control.id} className="grid gap-4 py-5 lg:grid-cols-12">
              <div className="lg:col-span-3">
                <p className="text-[13px] font-medium">{control.label}</p>
                <p className="mt-1 font-mono text-[11px] text-muted-foreground">{control.id}</p>
                <p className="mt-1 text-[11px] text-muted-foreground">
                  default {control.defaultValue} · {control.min} to {control.max}
                </p>
              </div>
              <div className="lg:col-span-5">
                <p className="label-mono">What it does</p>
                <p className="mt-2 text-sm leading-6 text-muted-foreground">
                  {control.whatItDoes}
                </p>
              </div>
              <div className="lg:col-span-4">
                <p className="label-mono">What you hear</p>
                <p className="mt-2 text-sm leading-6 text-muted-foreground">
                  {control.whatYouHear}
                </p>
              </div>
            </div>
          ))}
        </div>
      </section>

      <section>
        <h2 className="label-mono">Playing it from the keyboard</h2>
        <div className="mt-4 grid gap-8 lg:grid-cols-12">
          <div className="lg:col-span-5">
            <p className="text-sm leading-6 text-muted-foreground">
              The slot marked KEYS answers the piano roll across the whole range.
              A note in the middle plays the sample at the pitch it was recorded
              at; everything above and below is pitched from there, like a tape
              machine. Press KEYS on another slot to hand the keyboard over — the
              choice is saved with the project.
            </p>
            <p className="mt-4 text-sm leading-6 text-muted-foreground">
              Notes that arrive halfway through a block start halfway through the
              block, not at its edge, so the keyboard sits tight against the
              drums.
            </p>
          </div>
          <div className="lg:col-span-7">
            <ul className="divide-y divide-border border-y border-border">
              {SAMPLER_ROUTING.map((row) => (
                <li key={row.key} className="flex items-baseline justify-between gap-4 py-3">
                  <span className="text-[13px]">{row.key}</span>
                  <span className="font-mono text-[11px] text-muted-foreground">{row.value}</span>
                </li>
              ))}
            </ul>
          </div>
        </div>
      </section>

      <section>
        <h2 className="label-mono">How to build it</h2>
        <ol className="mt-4 divide-y divide-border border-y border-border">
          {STEPS.map((step, index) => (
            <li key={step.title} className="flex gap-5 py-5">
              <span className="font-mono text-[11px] tabular-nums text-muted-foreground">
                {String(index + 1).padStart(2, "0")}
              </span>
              <div className="min-w-0">
                <h3 className="text-sm font-bold tracking-tight">{step.title}</h3>
                <p className="mt-2 text-sm leading-6 text-muted-foreground">{step.body}</p>
              </div>
            </li>
          ))}
        </ol>
        <p className="mt-4 text-xs leading-6 text-muted-foreground">
          MP3 loading is one define in CMakeLists.txt (
          <span className="font-mono">JUCE_USE_MP3AUDIOFORMAT=1</span>). JUCE ships
          that decoder switched off, so without it the plugin loads WAV only.
        </p>
      </section>

      <section>
        <h2 className="label-mono">Check it by ear</h2>
        <div className="mt-4 divide-y divide-border border-y border-border">
          {LISTENING.map((item) => (
            <div key={item.title} className="py-5">
              <h3 className="text-sm font-bold tracking-tight">{item.title}</h3>
              <p className="mt-2 text-sm leading-6 text-muted-foreground">{item.body}</p>
            </div>
          ))}
        </div>
      </section>

      <section>
        <h2 className="label-mono">Source files</h2>
        <ul className="mt-4 divide-y divide-border border-y border-border">
          {SAMPLER_DOWNLOAD_FILES.map((file) => (
            <li key={file.path}>
              <a
                href={samplerDownloadUrl(file.path)}
                download
                className="group flex items-center justify-between gap-4 py-4 transition-colors hover:bg-accent/40"
              >
                <span className="flex min-w-0 items-center gap-3 pl-1">
                  <FileCode2 className="size-4 shrink-0 text-muted-foreground" />
                  <span className="min-w-0">
                    <span className="block truncate font-mono text-[12px]">{file.label}</span>
                    <span className="block truncate text-xs text-muted-foreground">
                      {file.note}
                    </span>
                  </span>
                </span>
                <Download className="mr-1 size-4 shrink-0 text-muted-foreground transition-colors group-hover:text-foreground" />
              </a>
            </li>
          ))}
        </ul>

        <div className="mt-6 border-t border-border pt-6">
          <p className="label-mono">Not in v1, on purpose</p>
          <ul className="mt-3 grid gap-2 text-xs leading-6 text-muted-foreground sm:grid-cols-2">
            {NOT_IN_V1.map((item) => (
              <li key={item} className="border-l border-border pl-3">
                {item}
              </li>
            ))}
          </ul>
        </div>
      </section>
    </div>
  );
}
