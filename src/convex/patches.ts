import { getAuthUserId } from "@convex-dev/auth/server";
import { ConvexError, v } from "convex/values";
import { mutation, query } from "./_generated/server";

/**
 * Saved Koola10 Synth patches.
 *
 * Every function here is scoped to the signed-in user, so one account can never
 * read or change another account's patches.
 */

const patchValues = {
  name: v.string(),
  wavetablePosition: v.number(),
  filterCutoff: v.number(),
  filterResonance: v.number(),
  attack: v.number(),
  decay: v.number(),
  sustain: v.number(),
  release: v.number(),
  outputGain: v.number(),
  notes: v.optional(v.string()),
};

/** All patches belonging to the signed-in user, newest first. */
export const list = query({
  args: {},
  handler: async (ctx) => {
    const userId = await getAuthUserId(ctx);
    if (userId === null) return [];

    return await ctx.db
      .query("synthPatches")
      .withIndex("by_user", (q) => q.eq("userId", userId))
      .order("desc")
      .collect();
  },
});

/** Saves a new patch for the signed-in user. */
export const create = mutation({
  args: patchValues,
  handler: async (ctx, args) => {
    const userId = await getAuthUserId(ctx);
    if (userId === null) {
      throw new ConvexError("You must be signed in to save a patch.");
    }

    return await ctx.db.insert("synthPatches", {
      userId,
      name: args.name.trim() || "Untitled patch",
      wavetablePosition: args.wavetablePosition,
      filterCutoff: args.filterCutoff,
      filterResonance: args.filterResonance,
      attack: args.attack,
      decay: args.decay,
      sustain: args.sustain,
      release: args.release,
      outputGain: args.outputGain,
      notes: args.notes,
    });
  },
});

/** Updates the knob values of a patch the signed-in user owns. */
export const update = mutation({
  args: {
    id: v.id("synthPatches"),
    ...patchValues,
  },
  handler: async (ctx, args) => {
    const userId = await getAuthUserId(ctx);
    if (userId === null) {
      throw new ConvexError("You must be signed in to edit a patch.");
    }

    const existing = await ctx.db.get(args.id);
    if (existing === null || existing.userId !== userId) {
      throw new ConvexError("Patch not found.");
    }

    const { id, ...values } = args;

    await ctx.db.patch(id, {
      name: values.name.trim() || existing.name,
      wavetablePosition: values.wavetablePosition,
      filterCutoff: values.filterCutoff,
      filterResonance: values.filterResonance,
      attack: values.attack,
      decay: values.decay,
      sustain: values.sustain,
      release: values.release,
      outputGain: values.outputGain,
      notes: values.notes,
    });
  },
});

/** Deletes a patch the signed-in user owns. */
export const remove = mutation({
  args: { id: v.id("synthPatches") },
  handler: async (ctx, args) => {
    const userId = await getAuthUserId(ctx);
    if (userId === null) {
      throw new ConvexError("You must be signed in to delete a patch.");
    }

    const existing = await ctx.db.get(args.id);
    if (existing === null || existing.userId !== userId) {
      throw new ConvexError("Patch not found.");
    }

    await ctx.db.delete(args.id);
  },
});
