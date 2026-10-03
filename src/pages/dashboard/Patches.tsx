import { useMutation, useQuery } from "convex/react";
import { Loader2, RotateCcw, Save, Trash2, Upload } from "lucide-react";
import { useMemo, useState } from "react";
import { toast } from "sonner";

import { api } from "@/convex/_generated/api";
import type { Id } from "@/convex/_generated/dataModel";
import { Button } from "@/components/ui/button";
import { Input } from "@/components/ui/input";
import { Slider } from "@/components/ui/slider";
import {
  DEFAULT_PATCH,
  PARAMETERS,
  PARAM_GROUPS,
  PATCH_KEY,
  type PatchValues,
  type SynthParam,
} from "@/lib/synth";

// --- Slider curve helpers -------------------------------------------------
// A cutoff slider on a linear scale would cram every useful low frequency into
// the first few percent of its travel, so log-curve parameters are mapped.

function toSlider(param: SynthParam, value: number) {
  if (param.curve !== "log") return value;

  const ratio = param.max / param.min;
  const t = Math.log(Math.max(value, param.min) / param.min) / Math.log(ratio);
  return Math.min(1, Math.max(0, t));
}

function fromSlider(param: SynthParam, t: number) {
  if (param.curve !== "log") return t;

  const ratio = param.max / param.min;
  return param.min * Math.pow(ratio, t);
}

type PatchDoc = PatchValues & {
  _id: Id<"synthPatches">;
  _creationTime: number;
  name: string;
  notes?: string;
};

function docToValues(doc: PatchDoc): PatchValues {
  return {
    wavetablePosition: doc.wavetablePosition,
    filterCutoff: doc.filterCutoff,
    filterResonance: doc.filterResonance,
    attack: doc.attack,
    decay: doc.decay,
    sustain: doc.sustain,
    release: doc.release,
    outputGain: doc.outputGain,
  };
}

export default function Patches() {
  const patches = useQuery(api.patches.list);
  const createPatch = useMutation(api.patches.create);
  const removePatch = useMutation(api.patches.remove);

  const [values, setValues] = useState<PatchValues>(DEFAULT_PATCH);
  const [name, setName] = useState("");
  const [saving, setSaving] = useState(false);

  const setValue = (key: keyof PatchValues, next: number) =>
    setValues((current) => ({ ...current, [key]: next }));

  const exportJson = useMemo(
    () => () => {
      const payload = {
        schema_version: "1.1.0",
        plugin: "koola10_synth",
        preset_name: name.trim() || "Untitled patch",
        parameters: Object.entries(PATCH_KEY).map(([id, key]) => ({
          id,
          value: Number(values[key].toFixed(4)),
        })),
      };

      const blob = new Blob([JSON.stringify(payload, null, 2)], {
        type: "application/json",
      });
      const url = URL.createObjectURL(blob);
      const link = document.createElement("a");
      link.href = url;
      link.download = `${(name.trim() || "koola10-patch").replace(/\s+/g, "-").toLowerCase()}.json`;
      link.click();
      URL.revokeObjectURL(url);

      toast("Patch exported", {
        description: "Unified Parameter Schema JSON downloaded.",
      });
    },
    [name, values],
  );

  const handleSave = async () => {
    setSaving(true);
    try {
      await createPatch({
        name: name.trim() || "Untitled patch",
        ...values,
      });
      setName("");
      toast("Patch saved", {
        description: "It now appears in your patch library.",
      });
    } catch (error) {
      toast("Could not save that patch", {
        description: error instanceof Error ? error.message : "Unknown error",
      });
    } finally {
      setSaving(false);
    }
  };

  const handleDelete = async (id: Id<"synthPatches">, label: string) => {
    try {
      await removePatch({ id });
      toast(`Deleted “${label}”`);
    } catch (error) {
      toast("Could not delete that patch", {
        description: error instanceof Error ? error.message : "Unknown error",
      });
    }
  };

  return (
    <div className="space-y-12">
      <header className="border-b border-border pb-8">
        <p className="label-mono">Patch library</p>
        <h1 className="mt-3 text-2xl font-bold tracking-tight">
          Set the eight knobs, then keep the result.
        </h1>
        <p className="mt-3 max-w-2xl text-sm leading-6 text-muted-foreground">
          These sliders mirror the plugin panel exactly, so a patch you save
          here is the same set of values you dial in on the VST3. Export it as
          Unified Parameter Schema JSON to hand it to the render pipeline.
        </p>
      </header>

      <div className="grid gap-14 lg:grid-cols-12">
        {/* ------------------------------------------------------------ */}
        <section className="lg:col-span-7">
          <div className="flex flex-col gap-4 border-b border-border pb-6 sm:flex-row sm:items-end sm:justify-between">
            <div className="w-full sm:max-w-xs">
              <label htmlFor="patch-name" className="label-mono">
                Patch name
              </label>
              <Input
                id="patch-name"
                value={name}
                onChange={(event) => setName(event.target.value)}
                placeholder="Warm hollow pad"
                className="mt-2 rounded-none"
              />
            </div>

            <div className="flex flex-wrap items-center gap-2">
              <Button
                type="button"
                variant="outline"
                className="rounded-none gap-2"
                onClick={exportJson}
              >
                <Upload className="size-3.5" />
                Export JSON
              </Button>
              <Button
                type="button"
                variant="outline"
                className="rounded-none gap-2"
                onClick={() => setValues(DEFAULT_PATCH)}
              >
                <RotateCcw className="size-3.5" />
                Reset
              </Button>
              <Button
                type="button"
                className="rounded-none gap-2"
                disabled={saving}
                onClick={handleSave}
              >
                {saving ? (
                  <Loader2 className="size-3.5 animate-spin" />
                ) : (
                  <Save className="size-3.5" />
                )}
                Save patch
              </Button>
            </div>
          </div>

          <div className="mt-10 space-y-12">
            {PARAM_GROUPS.map((group) => {
              const params = PARAMETERS.filter((p) => p.group === group.group);

              return (
                <section key={group.group}>
                  <div className="flex items-baseline justify-between gap-4">
                    <h2 className="label-mono">
                      Stage {group.dspStage} · {group.group}
                    </h2>
                  </div>
                  <p className="mt-2 text-xs leading-5 text-muted-foreground">
                    {group.blurb}
                  </p>

                  <div className="mt-6 space-y-7">
                    {params.map((param) => {
                      const key = PATCH_KEY[param.id];
                      const raw = values[key];
                      const sliderValue = toSlider(param, raw);
                      const isLog = param.curve === "log";

                      return (
                        <div key={param.id}>
                          <div className="flex items-baseline justify-between gap-4">
                            <label className="text-[13px] font-medium">
                              {param.label}
                            </label>
                            <span className="font-mono text-[12px] tabular-nums text-muted-foreground">
                              {param.format(raw)}
                            </span>
                          </div>

                          <Slider
                            className="mt-3"
                            min={isLog ? 0 : param.min}
                            max={isLog ? 1 : param.max}
                            step={isLog ? 0.0005 : param.step}
                            value={[sliderValue]}
                            onValueChange={([next]) =>
                              setValue(
                                key,
                                isLog
                                  ? fromSlider(param, next)
                                  : next,
                              )
                            }
                          />

                          <p className="mt-3 text-xs leading-5 text-muted-foreground">
                            {param.whatYouHear}
                          </p>
                        </div>
                      );
                    })}
                  </div>
                </section>
              );
            })}
          </div>
        </section>

        {/* ------------------------------------------------------------ */}
        <section className="lg:col-span-5">
          <div className="lg:sticky lg:top-20">
            <h2 className="label-mono">Saved patches</h2>

            {patches === undefined ? (
              <div className="mt-6 flex items-center gap-3 text-sm text-muted-foreground">
                <Loader2 className="size-4 animate-spin" />
                Loading your patches
              </div>
            ) : patches.length === 0 ? (
              <div className="mt-6 border-t border-border pt-6">
                <p className="text-sm leading-6 text-muted-foreground">
                  No patches yet. Set the knobs on the left, give the patch a
                  name, and save it — it will show up here for as long as your
                  account exists.
                </p>
              </div>
            ) : (
              <ul className="mt-6 divide-y divide-border border-y border-border">
                {(patches as PatchDoc[]).map((patch) => (
                  <li key={patch._id} className="py-4">
                    <div className="flex items-start justify-between gap-4">
                      <div className="min-w-0">
                        <p className="truncate text-[13px] font-medium">
                          {patch.name}
                        </p>
                        <p className="mt-1 font-mono text-[11px] tabular-nums text-muted-foreground">
                          {PARAMETERS.filter((p) => p.group === "Filter")
                            .map(
                              (p) =>
                                `${p.label} ${p.format(patch[PATCH_KEY[p.id]])}`,
                            )
                            .join(" · ")}
                        </p>
                        <p className="mt-1 font-mono text-[11px] tabular-nums text-muted-foreground">
                          {PARAMETERS.filter((p) => p.group === "Amplitude")
                            .map(
                              (p) =>
                                `${p.label} ${p.format(patch[PATCH_KEY[p.id]])}`,
                            )
                            .join(" · ")}
                        </p>
                      </div>

                      <div className="flex shrink-0 items-center gap-1">
                        <Button
                          type="button"
                          variant="ghost"
                          size="sm"
                          className="rounded-none text-muted-foreground hover:text-foreground"
                          onClick={() => {
                            setValues(docToValues(patch));
                            setName(patch.name);
                            toast(`Loaded “${patch.name}” into the designer`);
                          }}
                        >
                          Load
                        </Button>
                        <Button
                          type="button"
                          variant="ghost"
                          size="icon-sm"
                          aria-label={`Delete ${patch.name}`}
                          className="rounded-none text-muted-foreground hover:text-foreground"
                          onClick={() => handleDelete(patch._id, patch.name)}
                        >
                          <Trash2 className="size-3.5" />
                        </Button>
                      </div>
                    </div>
                  </li>
                ))}
              </ul>
            )}
          </div>
        </section>
      </div>
    </div>
  );
}
