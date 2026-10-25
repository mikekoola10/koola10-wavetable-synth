import { Download, FileCode2, Layers } from "lucide-react";

import { Button } from "@/components/ui/button";
import {
  DRUM_DOWNLOAD_FILES,
  DRUM_ZIP,
  FACTORY_PATTERN,
  GLOBAL_CONTROLS,
  PADS,
  PAD_KNOBS,
  SEQUENCER_FACTS,
  drumDownloadUrl,
} from "@/lib/drums";

const STEPS = [
  {
    title: "Unzip next to the synth",
    body: "So the folder lands at koola10-wavetable-synth\\koola10-drum-machine\\. It is a separate plugin with its own CMake project; nothing in the synth's plugin-juce folder moves.",
  },
  {
    title: "Open the drum machine's plugin-juce folder",
    body: "In Visual Studio: File → Open → Folder, and pick koola10-drum-machine\\plugin-juce (the one containing CMakeLists.txt).",
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
    title: "Find both builds",
    body: "The VST3 is copied to C:\\Program Files\\Common Files\\VST3\\Koola10 Drum Machine.vst3, and a Standalone .exe is built alongside it for testing without a DAW.",
  },
];

const LISTENING = [
  {
    title: "Sounds like a finished bed, not a demo",
    body: "A four-bar loop at about 130 BPM with kick, snare, hats and clap should hold a beat on its own. If it does not, the usual culprits are a snare decay that is too long, hats that are too loud, or a cutoff that is closed too far.",
  },
  {
    title: "The kick still punches with the synth's bass",
    body: "If the two turn to mud, pull the drum machine's Filter Cutoff down a little (or the bass's filter), instead of turning the kick down. The kick's weight is its whole job.",
  },
  {
    title: "Every knob does what its label says",
    body: "Turn each one on its own and listen. Every readout is a percentage except FILTER CUTOFF, which stays in kHz on purpose.",
  },
  {
    title: "The pattern survives a restart",
    body: "Close the project and reopen it. The grid and every knob should come back exactly where you left them.",
  },
];

export default function DrumMachine() {
  return (
    <div className="space-y-14">
      <header className="border-b border-border pb-8">
        <p className="label-mono">Koola10 Drum Machine · v1</p>
        <h1 className="mt-3 text-2xl font-bold tracking-tight">
          Eight synthesised pads and a sixteen-step grid.
        </h1>
        <p className="mt-3 max-w-2xl text-sm leading-6 text-muted-foreground">
          No samples to load, slice or lose. Every pad is generated while you
          play, the sequencer follows your DAW's tempo, and every readout is a
          percentage except the filter cutoff, which stays in kHz.
        </p>

        <div className="mt-6 flex flex-wrap items-center gap-3">
          <Button asChild className="rounded-none">
            <a href={DRUM_ZIP} download>
              <Layers className="size-4" />
              Download the drum machine (.zip)
            </a>
          </Button>
          <Button asChild variant="outline" className="rounded-none">
            <a href={drumDownloadUrl("README.md")} download>
              <FileCode2 className="size-4" />
              README.md
            </a>
          </Button>
          <span className="label-mono">Same repo, separate plugin</span>
        </div>
      </header>

      <section>
        <h2 className="label-mono">Build it</h2>
        <p className="mt-3 max-w-2xl text-sm leading-6 text-muted-foreground">
          Visual Studio 2022 with “Desktop development with C++”. Target is VST3
          plus Standalone on Windows x64, the same toolchain as Koola10 Synth.
        </p>

        <ol className="mt-6 divide-y divide-border border-y border-border">
          {STEPS.map((step, index) => (
            <li key={step.title} className="flex gap-6 py-5">
              <span className="mt-0.5 font-mono text-[11px] tabular-nums text-muted-foreground">
                {String(index + 1).padStart(2, "0")}
              </span>
              <div>
                <h3 className="text-sm font-bold tracking-tight">{step.title}</h3>
                <p className="mt-2 text-sm leading-6 text-muted-foreground">
                  {step.body}
                </p>
              </div>
            </li>
          ))}
        </ol>

        <pre className="mt-6 overflow-x-auto border border-border bg-card/40 p-4 text-[11px] leading-6 text-muted-foreground">
          <code>{`cd koola10-drum-machine\\plugin-juce
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release`}</code>
        </pre>
      </section>

      <section>
        <h2 className="label-mono">The eight pads</h2>
        <p className="mt-3 max-w-2xl text-sm leading-6 text-muted-foreground">
          Every pad answers the General MIDI drum note, so the FL Studio piano
          roll works without any mapping. Note velocity sets the pad level.
        </p>

        <div className="mt-6 divide-y divide-border border-y border-border">
          {PADS.map((pad) => (
            <div key={pad.id} className="grid gap-4 py-5 lg:grid-cols-12">
              <div className="lg:col-span-3">
                <p className="text-[13px] font-medium">{pad.name}</p>
                <p className="mt-1 font-mono text-[11px] text-muted-foreground">
                  note {pad.midiNote} ({pad.noteName})
                </p>
              </div>
              <div className="lg:col-span-5">
                <p className="label-mono">What it is</p>
                <p className="mt-2 text-sm leading-6 text-muted-foreground">
                  {pad.whatItIs}
                </p>
                <p className="mt-2 text-[11px] leading-5 text-muted-foreground">
                  {pad.synthesis}
                </p>
              </div>
              <div className="lg:col-span-4">
                <p className="label-mono">Try this</p>
                <p className="mt-2 text-sm leading-6 text-muted-foreground">
                  {pad.tryThis}
                </p>
              </div>
            </div>
          ))}
        </div>
      </section>

      <section>
        <h2 className="label-mono">Three knobs on every pad</h2>
        <div className="mt-4 grid gap-px bg-border sm:grid-cols-3">
          {PAD_KNOBS.map((knob) => (
            <div key={knob.label} className="bg-background p-6">
              <h3 className="text-sm font-bold tracking-tight">{knob.label}</h3>
              <p className="mt-3 text-sm leading-6 text-muted-foreground">
                {knob.whatItDoes}
              </p>
              <p className="mt-3 border-t border-border pt-3 text-sm leading-6 text-muted-foreground">
                {knob.whatYouHear}
              </p>
            </div>
          ))}
        </div>
      </section>

      <section>
        <h2 className="label-mono">Global controls</h2>
        <div className="mt-4 divide-y divide-border border-y border-border">
          {GLOBAL_CONTROLS.map((control) => (
            <div key={control.id} className="grid gap-4 py-5 lg:grid-cols-12">
              <div className="lg:col-span-3">
                <p className="text-[13px] font-medium">{control.label}</p>
                <p className="mt-1 font-mono text-[11px] text-muted-foreground">
                  {control.id}
                </p>
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
        <h2 className="label-mono">The sequencer</h2>
        <div className="mt-4 grid gap-px bg-border sm:grid-cols-2">
          {SEQUENCER_FACTS.map((fact) => (
            <div key={fact.title} className="bg-background p-6">
              <h3 className="text-sm font-bold tracking-tight">{fact.title}</h3>
              <p className="mt-3 text-sm leading-6 text-muted-foreground">{fact.body}</p>
            </div>
          ))}
        </div>

        <div className="mt-8 grid gap-8 lg:grid-cols-12">
          <div className="lg:col-span-5">
            <p className="label-mono">The pattern it ships with</p>
            <p className="mt-3 text-sm leading-6 text-muted-foreground">
              130 BPM, so the plugin makes a beat before you draw a single step.
              Sixteen steps, one bar.
            </p>
          </div>
          <div className="lg:col-span-7">
            <ul className="divide-y divide-border border-y border-border">
              {FACTORY_PATTERN.map((row) => (
                <li key={row.pad} className="flex items-baseline justify-between gap-4 py-3">
                  <span className="text-[13px]">{row.pad}</span>
                  <span className="font-mono text-[11px] tabular-nums text-muted-foreground">
                    {row.steps}
                  </span>
                </li>
              ))}
            </ul>
          </div>
        </div>
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
          {DRUM_DOWNLOAD_FILES.map((file) => (
            <li key={file.path}>
              <a
                href={drumDownloadUrl(file.path)}
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
        <p className="mt-6 text-xs leading-6 text-muted-foreground">
          Not in v1, on purpose: sample loading and slicing (that is the sampler,
          next in the rack), FM, keys and bass instruments, vocal processing,
          song mode and per-step velocity. The piano roll carries velocity
          instead.
        </p>
      </section>
    </div>
  );
}
