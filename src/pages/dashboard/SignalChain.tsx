import { ArrowDown } from "lucide-react";
import { motion } from "framer-motion";

import { PARAMETERS, SIGNAL_CHAIN } from "@/lib/synth";

export default function SignalChain() {
  return (
    <div className="space-y-12">
      <header className="border-b border-border pb-8">
        <p className="label-mono">Signal chain</p>
        <h1 className="mt-3 text-2xl font-bold tracking-tight">
          Where the sound is made, stage by stage.
        </h1>
        <p className="mt-3 max-w-2xl text-sm leading-6 text-muted-foreground">
          Every stage below is a separate, readable block in
          SynthEngine.cpp. The line numbers move as the file grows, so the code
          is labelled with the exact comment markers listed here instead.
        </p>
      </header>

      <div className="space-y-0">
        {SIGNAL_CHAIN.map((stage, index) => {
          const controls = PARAMETERS.filter((p) => p.dspStage === stage.stage);

          return (
            <motion.section
              key={stage.stage}
              initial={{ opacity: 0, y: 10 }}
              animate={{ opacity: 1, y: 0 }}
              transition={{ duration: 0.4, delay: index * 0.06 }}
            >
              <div className="grid gap-6 border-b border-border py-8 lg:grid-cols-12">
                <div className="lg:col-span-4">
                  <div className="flex items-center gap-3">
                    <span className="flex size-7 items-center justify-center border border-border text-[11px] tabular-nums text-muted-foreground">
                      {stage.stage}
                    </span>
                    <h2 className="text-sm font-bold tracking-tight">
                      {stage.name}
                    </h2>
                  </div>
                  <p className="mt-4 text-sm leading-6 text-muted-foreground">
                    {stage.detail}
                  </p>
                </div>

                <div className="lg:col-span-4">
                  <p className="label-mono">Find it in the source</p>
                  <code className="mt-3 block break-words border-l-2 border-border pl-4 text-[11px] leading-6 text-muted-foreground">
                    {stage.file}
                    <br />
                    <span className="text-foreground">
                      {stage.marker}
                    </span>
                    <br />
                    {stage.api}
                  </code>
                </div>

                <div className="lg:col-span-4">
                  <p className="label-mono">Controls owned by this stage</p>
                  <ul className="mt-3 space-y-2">
                    {controls.map((control) => (
                      <li key={control.id} className="text-[13px]">
                        <span className="font-medium">{control.label}</span>
                        <span className="ml-2 font-mono text-[11px] text-muted-foreground">
                          {control.id}
                        </span>
                      </li>
                    ))}
                  </ul>
                </div>
              </div>

              {index < SIGNAL_CHAIN.length - 1 && (
                <div className="flex justify-center py-3">
                  <ArrowDown className="size-3.5 text-muted-foreground" />
                </div>
              )}
            </motion.section>
          );
        })}
      </div>

      <section className="border-t border-border pt-8">
        <p className="label-mono">Output contract</p>
        <p className="mt-3 max-w-2xl text-sm leading-6 text-muted-foreground">
          After stage 4 the plugin writes straight into the host's stereo
          buffer. No wrapper format, no embedded project data, no proprietary
          container — so anything the plugin renders is plain PCM audio that a
          downstream asset pipeline can read as a normal .wav.
        </p>
      </section>
    </div>
  );
}
