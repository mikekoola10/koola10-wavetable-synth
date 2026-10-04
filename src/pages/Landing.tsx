import { motion } from "framer-motion";
import {
  ArrowRight,
  Download,
  FileCode2,
  Layers,
  SlidersHorizontal,
} from "lucide-react";
import type { ReactNode } from "react";
import { Link } from "react-router";

import { Button } from "@/components/ui/button";
import { useAuth } from "@/hooks/use-auth";
import {
  DOWNLOAD_FILES,
  SIGNAL_CHAIN,
  downloadUrl,
} from "@/lib/synth";

const EASE: [number, number, number, number] = [0.22, 1, 0.36, 1];

function Reveal({
  children,
  delay = 0,
  className,
}: {
  children: ReactNode;
  delay?: number;
  className?: string;
}) {
  return (
    <motion.div
      initial={{ opacity: 0, y: 14 }}
      whileInView={{ opacity: 1, y: 0 }}
      viewport={{ once: true, margin: "-60px" }}
      transition={{ duration: 0.6, ease: EASE, delay }}
      className={className}
    >
      {children}
    </motion.div>
  );
}

/** A monochrome SVG sketch of the plugin: one waveform, one hairline grid. */
function SynthPreview() {
  const wave = Array.from({ length: 120 }, (_, i) => {
    const t = i / 119;
    const y =
      Math.sin(t * Math.PI * 2 * 3) * 0.55 +
      Math.sin(t * Math.PI * 2 * 7) * 0.22 +
      Math.sin(t * Math.PI * 2 * 11) * 0.09;
    return 46 - y * 26;
  });

  const path = wave.map((y, i) => `${i === 0 ? "M" : "L"}${i * 4} ${y.toFixed(2)}`).join(" ");

  return (
    <div className="border border-border bg-card/40 p-1">
      <div className="flex items-center justify-between border-b border-border px-3 py-2">
        <span className="label-mono">Koola10 Synth</span>
        <span className="label-mono">VST3 · v2</span>
      </div>

      <div className="border-b border-border px-3 py-4">
        <svg viewBox="0 0 476 92" className="h-24 w-full" aria-hidden="true">
          <line x1="0" y1="46" x2="476" y2="46" stroke="currentColor" strokeOpacity="0.12" />
          <path d={path} fill="none" stroke="currentColor" strokeWidth="1.4" strokeLinejoin="round" />
        </svg>
      </div>

      <div className="grid grid-cols-4 divide-x divide-border">
        {[
          { label: "Position", value: "0 %" },
          { label: "Cutoff", value: "2.50 kHz" },
          { label: "Resonance", value: "0 %" },
          { label: "Release", value: "47 %" },
        ].map((knob) => (
          <div key={knob.label} className="flex flex-col items-center gap-2 px-2 py-4">
            <svg viewBox="0 0 40 40" className="size-9" aria-hidden="true">
              <circle
                cx="20"
                cy="20"
                r="15"
                fill="none"
                stroke="currentColor"
                strokeOpacity="0.16"
                strokeDasharray="70 24"
                strokeDashoffset="-12"
                strokeWidth="1.4"
                transform="rotate(90 20 20)"
              />
              <circle
                cx="20"
                cy="20"
                r="15"
                fill="none"
                stroke="currentColor"
                strokeDasharray="26 68"
                strokeDashoffset="-12"
                strokeWidth="1.8"
                transform="rotate(90 20 20)"
              />
            </svg>
            <span className="label-mono">{knob.label}</span>
            <span className="text-xs tabular-nums text-muted-foreground">{knob.value}</span>
          </div>
        ))}
      </div>
    </div>
  );
}

export default function Landing() {
  const { isAuthenticated } = useAuth();
  const studioHref = isAuthenticated ? "/dashboard" : "/auth?returnTo=%2Fdashboard";

  return (
    <div className="dark min-h-screen bg-background text-foreground">
      {/* ---------------------------------------------------------------- */}
      <header className="sticky top-0 z-40 border-b border-border bg-background/85 backdrop-blur">
        <div className="mx-auto flex h-14 w-full max-w-6xl items-center justify-between px-6">
          <Link to="/" className="flex items-center gap-2">
            <span className="size-4 border border-foreground" />
            <span className="text-sm font-bold tracking-tight">Koola10 Synth</span>
          </Link>

          <nav className="hidden items-center gap-8 md:flex">
            <a href="#signal-chain" className="label-mono hover:text-foreground">
              Signal chain
            </a>
            <a href="#library" className="label-mono hover:text-foreground">
              Library
            </a>
            <a href="#knobs" className="label-mono hover:text-foreground">
              Knobs
            </a>
            <a href="#downloads" className="label-mono hover:text-foreground">
              Downloads
            </a>
            <a href="#build" className="label-mono hover:text-foreground">
              Build
            </a>
          </nav>

          <Button asChild size="sm" variant="outline" className="rounded-none">
            <Link to={studioHref}>
              {isAuthenticated ? "Open studio" : "Design a patch"}
              <ArrowRight className="size-3.5" />
            </Link>
          </Button>
        </div>
      </header>

      <main className="mx-auto w-full max-w-6xl px-6">
        {/* -------------------------------------------------------------- */}
        <section className="grid gap-14 py-20 md:grid-cols-12 md:py-28">
          <div className="md:col-span-6">
            <Reveal>
              <p className="label-mono">SpiralSynth AI · Instrument layer</p>
            </Reveal>
            <Reveal delay={0.06}>
              <h1 className="mt-6 text-4xl font-bold leading-[1.05] tracking-tight sm:text-5xl">
                A wavetable synthesizer that tells you what every knob does.
              </h1>
            </Reveal>
            <Reveal delay={0.12}>
              <p className="mt-6 max-w-xl text-[15px] leading-7 text-muted-foreground">
                Koola10 Synth is a JUCE VST3 instrument built in four honest
                stages: a frame-based wavetable oscillator, a low-pass filter,
                an ADSR amplitude envelope, and a master level. Eight labelled
                knobs, 24 factory waves, a searchable browser and eight presets
                — and nothing hidden behind them.
              </p>
            </Reveal>
            <Reveal delay={0.18}>
              <div className="mt-9 flex flex-wrap items-center gap-3">
                <Button asChild className="rounded-none">
                  <Link to={studioHref}>
                    {isAuthenticated ? "Open your studio" : "Design a patch"}
                    <ArrowRight className="size-4" />
                  </Link>
                </Button>
                <Button asChild variant="outline" className="rounded-none">
                  <a href="#downloads">
                    <Download className="size-4" />
                    Download the source
                  </a>
                </Button>
              </div>
            </Reveal>
            <Reveal delay={0.24}>
              <div className="mt-10 flex flex-wrap gap-x-10 gap-y-3 border-t border-border pt-6">
                {[
                  ["8", "labelled parameters"],
                  ["24", "factory waves"],
                  ["2048", "samples per frame"],
                ].map(([value, label]) => (
                  <div key={label}>
                    <p className="text-2xl font-bold tabular-nums tracking-tight">{value}</p>
                    <p className="label-mono mt-1">{label}</p>
                  </div>
                ))}
              </div>
            </Reveal>
          </div>

          <Reveal delay={0.1} className="md:col-span-6 md:pt-6">
            <SynthPreview />
          </Reveal>
        </section>

        {/* -------------------------------------------------------------- */}
        <section id="signal-chain" className="border-t border-border py-20">
          <Reveal>
            <p className="label-mono">Signal chain</p>
            <h2 className="mt-4 max-w-2xl text-2xl font-bold tracking-tight sm:text-3xl">
              Audio travels through four stages, in this order.
            </h2>
          </Reveal>

          <div className="mt-12 grid gap-px bg-border sm:grid-cols-2 lg:grid-cols-4">
            {SIGNAL_CHAIN.map((stage, index) => (
              <Reveal key={stage.stage} delay={index * 0.06} className="bg-background">
                <div className="flex h-full flex-col gap-4 p-6">
                  <div className="flex items-center gap-3">
                    <span className="flex size-6 items-center justify-center border border-border text-[10px] tabular-nums text-muted-foreground">
                      {stage.stage}
                    </span>
                    <span className="label-mono">{stage.name}</span>
                  </div>
                  <p className="flex-1 text-sm leading-6 text-muted-foreground">{stage.detail}</p>
                  <code className="block break-words border-t border-border pt-4 text-[11px] leading-5 text-muted-foreground">
                    {stage.file}
                    <br />
                    {stage.api}
                  </code>
                </div>
              </Reveal>
            ))}
          </div>
        </section>

        {/* -------------------------------------------------------------- */}
        <section id="library" className="border-t border-border py-20">
          <Reveal>
            <p className="label-mono">Sound library</p>
            <h2 className="mt-4 max-w-2xl text-2xl font-bold tracking-tight sm:text-3xl">
              A browser, 24 factory waves and eight presets — built in.
            </h2>
          </Reveal>

          <div className="mt-12 grid gap-px bg-border sm:grid-cols-3">
            {[
              [
                "24 factory waves",
                "Sub, Bass, Lead, Pad-Keys and FX, embedded in the plugin with juce_add_binary_data so there are no loose files to lose.",
              ],
              [
                "Searchable browser",
                "A slim left-hand panel groups every wave by category and filters by name as you type. Your own .wav files land in a User group.",
              ],
              [
                "Presets",
                "Eight factory presets recall a wave plus every knob in one click, and Save / Load round-trips the full state to a .koola10preset file.",
              ],
            ].map(([title, description], index) => (
              <Reveal key={title} delay={index * 0.06} className="bg-background">
                <div className="h-full p-6">
                  <h3 className="text-sm font-bold tracking-tight">{title}</h3>
                  <p className="mt-3 text-sm leading-6 text-muted-foreground">
                    {description}
                  </p>
                </div>
              </Reveal>
            ))}
          </div>
        </section>

        {/* -------------------------------------------------------------- */}
        <section id="knobs" className="border-t border-border py-20">
          <div className="grid gap-12 lg:grid-cols-12">
            <Reveal className="lg:col-span-4">
              <p className="label-mono">The knobs</p>
              <h2 className="mt-4 text-2xl font-bold tracking-tight sm:text-3xl">
                Every control, in plain English.
              </h2>
              <p className="mt-5 text-[15px] leading-7 text-muted-foreground">
                Each knob is labelled on the plugin panel and in the source. Here
                is what it changes, and what you actually hear when you turn it.
              </p>
              <div className="mt-8 flex items-start gap-3 border-t border-border pt-6 text-sm text-muted-foreground">
                <SlidersHorizontal className="mt-0.5 size-4 shrink-0" />
                <span>
                  All eight parameters are automatable and recorded in
                  shared/parameter_schema.json.
                </span>
              </div>
            </Reveal>

            <div className="lg:col-span-8">
              <div className="grid gap-px bg-border sm:grid-cols-2">
                {[
                  ["Wavetable Position", "Slides through the frames of the loaded table. Low is round and hollow, high is bright and buzzy."],
                  ["Filter Cutoff", "Where the low-pass starts removing energy. Down goes dark and muffled, up opens to full brightness."],
                  ["Filter Resonance", "Emphasis at the cutoff. A whistle appears and the note turns vocal; pushed far it rings on its own."],
                  ["Attack", "How long the note takes to reach full volume. Near zero is a click, a few hundred milliseconds is a swell."],
                  ["Decay", "How long to fall from full volume to the sustain level. Short is a knock, long is a lazy slide."],
                  ["Sustain", "The level held while the key is down. Full stays loud like an organ; zero dies away under your finger."],
                  ["Release", "How long the note fades after you let go. Short stops dead, long leaves a tail hanging in the air."],
                  ["Master Level", "Final volume after everything else, so you can match the synth against the rest of a project."],
                ].map(([label, description], index) => (
                  <Reveal key={label} delay={index * 0.03} className="bg-background">
                    <div className="h-full p-6">
                      <h3 className="text-sm font-bold tracking-tight">{label}</h3>
                      <p className="mt-3 text-sm leading-6 text-muted-foreground">
                        {description}
                      </p>
                    </div>
                  </Reveal>
                ))}
              </div>
            </div>
          </div>
        </section>

        {/* -------------------------------------------------------------- */}
        <section id="downloads" className="border-t border-border py-20">
          <div className="grid gap-12 lg:grid-cols-12">
            <Reveal className="lg:col-span-4">
              <p className="label-mono">Downloads</p>
              <h2 className="mt-4 text-2xl font-bold tracking-tight sm:text-3xl">
                Everything you need to build it.
              </h2>
              <p className="mt-5 text-[15px] leading-7 text-muted-foreground">
                Drop these files into your existing koola10-wavetable-synth
                repo, at exactly the paths shown. Nothing else in the project
                needs to move.
              </p>
              <div className="mt-8 border-t border-border pt-6">
                <p className="label-mono">Render path</p>
                <p className="mt-3 text-sm leading-6 text-muted-foreground">
                  Standard VST3 stereo audio out. No proprietary formats, no
                  proprietary asset wrappers — rendered audio is plain PCM that
                  any downstream pipeline can read.
                </p>
              </div>
            </Reveal>

            <Reveal delay={0.08} className="lg:col-span-8">
              <ul className="divide-y divide-border border-y border-border">
                {DOWNLOAD_FILES.map((file) => (
                  <li key={file.path}>
                    <a
                      href={downloadUrl(file.path)}
                      download
                      className="group flex items-center justify-between gap-4 py-4 transition-colors hover:bg-accent/40"
                    >
                      <span className="flex min-w-0 items-center gap-3 pl-1">
                        <FileCode2 className="size-4 shrink-0 text-muted-foreground" />
                        <span className="min-w-0">
                          <span className="block truncate font-mono text-[12px]">
                            {file.label}
                          </span>
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
              <div className="mt-6 flex flex-wrap items-center gap-3">
                <Button asChild variant="outline" className="rounded-none">
                  <a href="/koola10-synth-v2.zip" download>
                    <Layers className="size-4" />
                    Download all files (.zip)
                  </a>
                </Button>
                <span className="label-mono">Single archive, same layout</span>
              </div>
            </Reveal>
          </div>
        </section>

        {/* -------------------------------------------------------------- */}
        <section id="build" className="border-t border-border py-20">
          <div className="grid gap-12 lg:grid-cols-12">
            <Reveal className="lg:col-span-4">
              <p className="label-mono">Build</p>
              <h2 className="mt-4 text-2xl font-bold tracking-tight sm:text-3xl">
                Windows, Visual Studio 2022.
              </h2>
              <p className="mt-5 text-[15px] leading-7 text-muted-foreground">
                CMake generates the project for you, so there is no solution
                file to maintain. The first configure downloads JUCE and takes a
                few minutes.
              </p>
              <Button asChild variant="ghost" className="mt-6 rounded-none px-0">
                <Link to={studioHref}>
                  Save patches while you build
                  <ArrowRight className="size-4" />
                </Link>
              </Button>
            </Reveal>

            <div className="lg:col-span-8">
              <ol className="divide-y divide-border border-y border-border">
                {[
                  ["Install Visual Studio 2022", "Tick the “Desktop development with C++” workload. That one box includes the compiler and CMake."],
                  ["Open the plugin-juce folder", "In Visual Studio: File → Open → Folder, and choose the folder containing CMakeLists.txt."],
                  ["Let CMake configure", "Visual Studio starts on its own. The first run downloads JUCE, so it needs an internet connection."],
                  ["Build All", "Build → Build All (Ctrl+Shift+B). Use the Release configuration for real music making."],
                  ["Find the plugin", "It is copied to C:\\Program Files\\Common Files\\VST3\\Koola10 Synth.vst3, and a Standalone app is built alongside it."],
                ].map(([title, body], index) => (
                  <Reveal key={title} delay={index * 0.04}>
                    <li className="flex gap-6 py-5">
                      <span className="mt-0.5 text-[11px] tabular-nums text-muted-foreground">
                        {String(index + 1).padStart(2, "0")}
                      </span>
                      <div>
                        <h3 className="text-sm font-bold tracking-tight">{title}</h3>
                        <p className="mt-2 text-sm leading-6 text-muted-foreground">{body}</p>
                      </div>
                    </li>
                  </Reveal>
                ))}
              </ol>
              <p className="mt-6 text-xs leading-6 text-muted-foreground">
                The full walkthrough, a code map for each DSP stage, and a
                knob-by-knob listening guide are in README.md.
              </p>
            </div>
          </div>
        </section>
      </main>

      {/* ---------------------------------------------------------------- */}
      <footer className="border-t border-border">
        <div className="mx-auto flex w-full max-w-6xl flex-col gap-4 px-6 py-8 sm:flex-row sm:items-center sm:justify-between">
          <p className="label-mono">Koola10 Synth · SpiralSynth AI</p>
          <div className="flex items-center gap-6">
            <a href="#downloads" className="label-mono hover:text-foreground">
              Source files
            </a>
            <Link to={studioHref} className="label-mono hover:text-foreground">
              Studio
            </Link>
          </div>
        </div>
      </footer>
    </div>
  );
}
