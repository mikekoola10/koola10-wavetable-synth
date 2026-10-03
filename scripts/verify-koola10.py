import json
import re
import sys

ok = True


def fail(msg):
    global ok
    ok = False
    print("FAIL:", msg)


schema = json.load(open("public/koola10-synth/shared/parameter_schema.json"))
schema_ids = [p["id"] for p in schema["parameters"]]

hdr = open("public/koola10-synth/plugin-juce/Source/PluginProcessor.h").read()
cpp = open("public/koola10-synth/plugin-juce/Source/PluginProcessor.cpp").read()
eng = open("public/koola10-synth/plugin-juce/Source/SynthEngine.cpp").read()
ts = open("src/lib/synth.ts").read()

# 1. ParamIDs declared in the header
declared = dict(re.findall(r'constexpr\s+const\s+char\s*\*\s*(\w+)\s*=\s*"([^"]+)"', hdr))
print("header ParamIDs:", sorted(declared.values()))
missing_hdr = [i for i in schema_ids if i not in declared.values()]
if missing_hdr:
    fail("ids in schema but not declared in PluginProcessor.h: %s" % missing_hdr)

# 2. Every ParamIDs::<name> used in the cpp resolves to a declared symbol
used = set(re.findall(r"ParamIDs::(\w+)", cpp))
unknown = sorted(u for u in used if u not in declared)
if unknown:
    fail("ParamIDs::%s used in .cpp but not declared" % unknown)
print("ParamIDs:: references resolved:", sorted(used))

# 3. createParameterLayout declares one ParameterID per schema parameter
layout = re.findall(r"juce::ParameterID\s*\{", cpp)
print("juce::ParameterID occurrences:", len(layout))
if len(layout) != len(schema_ids):
    fail("expected %d ParameterIDs, found %d" % (len(schema_ids), len(layout)))

# 4. One "STAGE n" marker in SynthEngine.cpp per dsp_stage in the schema
stages = sorted({p["dsp_stage"] for p in schema["parameters"]})
for s in stages:
    if ("STAGE %d" % s) not in eng:
        fail("SynthEngine.cpp has no \"STAGE %d\" marker" % s)
print("engine stage markers present for stages:", stages)

# 5. Web metadata parity with the schema ids
ts_ids = re.findall(r'id:\s*"([a-z_]+)"', ts)
web_missing = [i for i in schema_ids if i not in ts_ids]
if web_missing:
    fail("schema ids missing from src/lib/synth.ts: %s" % web_missing)
print("synth.ts parameter ids:", sorted(set(ts_ids) & set(schema_ids)))

print("RESULT:", "PASS" if ok else "FAIL")
sys.exit(0 if ok else 1)