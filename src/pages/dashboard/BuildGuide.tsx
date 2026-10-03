import { Download, FileCode2 } from "lucide-react";

import { Button } from "@/components/ui/button";
import {
  DOWNLOAD_FILES,
  PARAMETERS,
  PARAM_GROUPS,
  downloadUrl,
} from "@/lib/synth";

const STEPS = [
  {
    title: "Install Visual Studio 2022",
    body: "Tick the “Desktop development with C++” workload during setup. That single box brings the compiler and CMake with it.",
  },
  {
    title: "Open the plugin-juce folder",
    body: "In Visual Studio choose File → Open → Folder and pick the folder containing CMakeLists.txt. Keep the path simple and outside OneDrive.",
  },
  {
    title: "Let CMake configure",
    body: "Visual Studio sees CMakeLists.txt and starts on its own. The first configure downloads JUCE, so it needs an internet connection and a few minutes.",
  },
  {
    title: "Build All",
    body: "Build → Build All, or Ctrl+Shift+B. Choose the Release configuration for music making; Debug is only for stepping through code.",
  },
  {
    title: "Find the plugin",
    body: "COPY_PLUGIN_AFTER_BUILD copies it to C:\\Program Files\\Common Files\\VST3\\Koola10 Synth.vst3. A Standalone .exe is built alongside it for testing without a DAW.",
  },
];

const TROUBLE = [
  [
    "CMake generation failed",
    "Almost always the JUCE download. Check your connection and that git is on your PATH.",
  ],
  [
    "No sound in the DAW",
    "Play notes from the piano roll — the synth does not drone on its own. Then check Sustain is above 0 %, Cutoff is up, and Master Level is not pulled down.",
  ],
  [
    "The plugin does not appear",
    "Confirm the .vst3 folder is in the VST3 directory, then rescan VST3 plugins (not VST2) in your DAW.",
  ],
  [
    "Loading a .wav does nothing",
    "Use standard PCM or 32-bit float .wav. Compressed formats are not supported.",
  ],
];

export default function BuildGuide() {
  return (
    <div className="space-y-14">
      <header className="border-b border-border pb-8">
        <p className="label-mono">Build guide</p>
        <h1 className="mt-3 text-2xl font-bold tracking-tight">
          Windows, Visual Studio 2022, five steps.
        </h1>
        <p className="mt-3 max-w-2xl text-sm leading-6 text-muted-foreground">
          The complete walkthrough — including a code map and a knob-by-knob
          listening guide — is in README.md. This is the short version.
        </p>
      </header>

      <section>
        <h2 className="label-mono">Build steps</h2>
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
          <code>{`cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release`}</code>
        </pre>
      </section>

      <section>
        <h2 className="label-mono">Knob reference</h2>
        <p className="mt-3 max-w-2xl text-sm leading-6 text-muted-foreground">
          What each control changes, and what you hear when you move it.
        </p>

        <div className="mt-6 space-y-10">
          {PARAM_GROUPS.map((group) => (
            <div key={group.group}>
              <h3 className="label-mono">
                Stage {group.dspStage} · {group.group}
              </h3>
              <div className="mt-4 divide-y divide-border border-y border-border">
                {PARAMETERS.filter((p) => p.group === group.group).map((param) => (
                  <div
                    key={param.id}
                    className="grid gap-4 py-5 lg:grid-cols-12"
                  >
                    <div className="lg:col-span-4">
                      <p className="text-[13px] font-medium">{param.label}</p>
                      <p className="mt-1 font-mono text-[11px] text-muted-foreground">
                        {param.id}
                      </p>
                      <p className="mt-1 text-[11px] text-muted-foreground">
                        default {param.format(param.default)} · {param.min} to{" "}
                        {param.max} {param.unit}
                      </p>
                    </div>
                    <div className="lg:col-span-4">
                      <p className="label-mono">What it does</p>
                      <p className="mt-2 text-sm leading-6 text-muted-foreground">
                        {param.whatItDoes}
                      </p>
                    </div>
                    <div className="lg:col-span-4">
                      <p className="label-mono">What you hear</p>
                      <p className="mt-2 text-sm leading-6 text-muted-foreground">
                        {param.whatYouHear}
                      </p>
                    </div>
                  </div>
                ))}
              </div>
            </div>
          ))}
        </div>
      </section>

      <section className="grid gap-10 border-t border-border pt-10 lg:grid-cols-2">
        <div>
          <h2 className="label-mono">Loading your own wavetable</h2>
          <ul className="mt-4 space-y-3 text-sm leading-6 text-muted-foreground">
            <li>
              Click LOAD .WAV and pick a standard .wav. Surge, Serum and Vital
              all export wavetables as plain .wav, so those work directly.
            </li>
            <li>
              The engine tests frame sizes of 2048, 1024, 512, 256, 128 and 64
              samples and takes the first one that divides the file into 1 to
              256 whole frames.
            </li>
            <li>
              A single-cycle waveform becomes a one-frame table, and the
              Position knob then has nothing to travel across.
            </li>
            <li>
              Stereo files are averaged down to mono, because a wavetable is a
              shape rather than a stereo image.
            </li>
          </ul>
        </div>

        <div>
          <h2 className="label-mono">If something goes wrong</h2>
          <dl className="mt-4 space-y-4">
            {TROUBLE.map(([problem, fix]) => (
              <div key={problem} className="border-l-2 border-border pl-4">
                <dt className="text-[13px] font-medium">{problem}</dt>
                <dd className="mt-1 text-sm leading-6 text-muted-foreground">
                  {fix}
                </dd>
              </div>
            ))}
          </dl>
        </div>
      </section>

      <section className="border-t border-border pt-10">
        <h2 className="label-mono">Source files</h2>
        <p className="mt-3 max-w-2xl text-sm leading-6 text-muted-foreground">
          Drop these into your existing fl-studio-plugins repo at exactly the
          paths shown. Nothing else in the project needs to move.
        </p>

        <ul className="mt-6 divide-y divide-border border-y border-border">
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

        <Button asChild variant="outline" className="mt-6 rounded-none">
          <a href="/koola10-synth-v1.zip" download>
            <Download className="size-4" />
            Download all files (.zip)
          </a>
        </Button>
      </section>
    </div>
  );
}
