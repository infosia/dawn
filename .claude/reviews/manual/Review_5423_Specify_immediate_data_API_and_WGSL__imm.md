# [GitHub] CL/PR 5423: Specify immediate data API and WGSL <immediate> address space

## Metadata Summary
- **Author**: shaoboyan091
- **Status**: open
- **URL**: https://github.com/gpuweb/gpuweb/pull/5423

## General PR Comments
**General Comment** - @github-actions[bot] (2025-10-29 08:54:11):
> Previews, as seen when this [build job](https://github.com/gpuweb/gpuweb/actions/runs/20116438101) started (6cc7322946f70e4dacb07bdff3cab12d1d2ba701):
> [**WebGPU**](https://raw.githack.com/gpuweb/gpuweb-pr-previews/850076bca292241cdd0cb0b7b1d3360efbc13284/gpuweb/gpuweb/5423/) <sub>[**`webgpu.idl`**](https://raw.githubusercontent.com/gpuweb/gpuweb-pr-previews/850076bca292241cdd0cb0b7b1d3360efbc13284/gpuweb/gpuweb/5423/webgpu.idl.txt) | [**Explainer**](https://raw.githack.com/gpuweb/gpuweb-pr-previews/850076bca292241cdd0cb0b7b1d3360efbc13284/gpuweb/gpuweb/5423/explainer/) | [**Correspondence Reference**](https://raw.githack.com/gpuweb/gpuweb-pr-previews/850076bca292241cdd0cb0b7b1d3360efbc13284/gpuweb/gpuweb/5423/correspondence/)</sub>
> [**WGSL**](https://raw.githack.com/gpuweb/gpuweb-pr-previews/850076bca292241cdd0cb0b7b1d3360efbc13284/gpuweb/gpuweb/5423/wgsl/) <sub>[**`grammar.js`**](https://raw.githubusercontent.com/gpuweb/gpuweb-pr-previews/850076bca292241cdd0cb0b7b1d3360efbc13284/gpuweb/gpuweb/5423/wgsl/grammar/grammar.js) | [**`wgsl.lalr.txt`**](https://raw.githubusercontent.com/gpuweb/gpuweb-pr-previews/850076bca292241cdd0cb0b7b1d3360efbc13284/gpuweb/gpuweb/5423/wgsl/wgsl.lalr.txt)</sub>
> <!--
> pr;head;sha;base
> 5423;shaoboyan091/gpuweb;6cc7322946f70e4dacb07bdff3cab12d1d2ba701;gpuweb/gpuweb
> -->

**General Comment** - @jimblandy (2025-10-29 17:58:21):
> <details>
> <summary>minutes from API committee meeting 2025-10-29</summary>
> <ul>
> <li>KN: Everything should be more or less done in the proposal. Is there anything else people know they want fixed before it moves forward? Shaobo has begun work on the spec PR.</li>
> <li>JB: Does the proposal include the word-granularity initialization checks that we agreed to in Toronto?</li>
> <li>KN: Yes</li>
> <li>JB: Okay, cool. Then I’m not aware of any other concerns that would block drafting a PR against the spec.</li>
> </ul>
> </details>

**General Comment** - @mwyrzykowski (2025-11-04 17:46:09):
> > Copying my other requests from #5424
> > 
> > There ought to be either a language feature or enable extension as part of this specification.
> 
> Would prefer not using an enable extension since this is supported or can be easily emulated by all core devices (and compat too I think).
> 
> I.e., WGSL language extension would be my preference.

**General Comment** - @mwyrzykowski (2025-11-05 17:13:18):
> I can't make the meeting today but this is 👍 from Apple's side. Metal doesn't have the concept of immediates and there was some discussion about this taking up a buffer slot. If the UA is using argument buffers then it would not, since a slot is already needed for dynamic data and you can reuse / share the same slot for immediates. Basically any transient data (dynamic offsets, immediates, other constants) can be passed in a single call to set[Vertex/Fragment]Bytes.
> 
> But we are also fine with this taking up a slot. This allows UAs to not have to migrate to ABs for everything for their Metal backends. I suppose its a tradeoff but either way is 👍 from Apple's side given our AB implementation is not really impacted either way.

**General Comment** - @jimblandy (2025-11-06 22:19:54):
> <details>
> <summary>minutes from WGSL committee meeting 2025-11-4</summary>
> <ul>
> <li>JB: PR for specifying immediate data. Understanding is this is a popular thing in graphics APIS. Think a lot of graphics folks would be happy if it was in the API and shipped. So, looks like most of the conversation has been Kai.</li>
> <li>DN: This is a way of getting constant data into shader that is uniform. Popular because it's fast, fewer indirections. Because it's fast, it's tiny. Feature allows a single variable in the &quot;immediate&quot; address space which is declared at module scope, it's uniform. From a WGSL perspective, slides in with other addres spaces. Most restrictions are API side. Make sure data gets in (api call) part of the binding pipeline descriptor to say space size. Validation for space side on api side covers the variable in the WGSL shader. Single immediate space of memory used for all the shaders in a given pipeline (unified model). THere is some validation about which 32-bit words have to be set in order to supply data. Don't have to write padding. Discussion about the imprecision of padding bytes. We dont' define padding or the inverse in a structure. Kind of implied. We could/recommend cleaning that up after this lands. Otherwise, editorial feedback. TLDR; fast uniform data access in shader.</li>
> <li>JB: Restriction against arrays, no arrays in the value, not even in struct. Is the reason that the platform level instructions for accessing odn't allow dynamic offsets?</li>
> <li>AB: Vulkan has a restriction that it's <a href="https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#interfaces-resources-pushconst">dynamically uniform</a></li>
> <li>JB: Do mention the WGSL out of bounds rules, if there are no arrays then that only applies to vectors and matrices. So, guess what's going on is the WebGPU backend needs to lift dynamic indexing of vectors and matrices in the immediate data into something. LIfting to a variable and indexing into that or something</li>
> <li>DN: That sounds right. Saw out of bounds doesn't really apply but you're right vector and matrices does exercise that.</li>
> <li>JB: Otherwise, editorial comments. Only remaining interesting thing is question of render bundles. Way it's specified that after bundle runs the immediate data is considered uninitialized. Chose that because it's the most restrictive and easiest to implement. If decide later due to render bundle transparency can consider relaxing this restriction. Want to start most restrictive and allow more over time. What is the status of this for CTS implementation and Dawn implementation.</li>
> <li>DS: Think the implementation is done, modulo bug fixes. We use this mechanism to pass side channel data for Dawn. Don’t think CTS is started.</li>
> <li>JB: Precedent I have in mind, when talking about subgroup_id, we said it looks good, and is accepted but wait for CTS before merging to main. Think the same reasoning applies here. Want to make sure we have CTS passing and the benefit from CTS before going ahead and shipping. MW has comment would prefer not using enable as it can be supported or emulated everywhere.</li>
> <li>DN: On WGSL side because it's a new feature we want enable or feature so we can write skipping conformance tests. Agree with MW that this should be everywhere, question in my mind is emulation steals uniform buffer slot, do we worry or need wording in API that the limits possibly get eaten into. Not a language concern, but an API concern.</li>
> <li>JB: So as far as WGSL is concerned … ok. How does this pr phrase it now, just no extension at all? Ok, seems like a good thing to fix.</li>
> <li>MW: Perhaps an issue for API meeting. Seems the only backend which doesn't support it is metal and there are ways to do it without taking a buffer slot (probably outside the scope of this meeting)</li>
> <li>JB: Then not a lot of discussion for this meeting. So, resolved on language extension?</li>
> <li>MW: &lt;thumbs up&gt;</li>
> <li>DN: That's my preference</li>
> <li>JB: Mozilla is thumbs up.</li>
> <li><strong>RESOLUTION: Make it a language extension, PR is good, needs CTS before landing PR</strong></li>
> </ul>
> </details>

**General Comment** - @Kangz (2025-11-12 15:07:47):
> > There ought to be either a language feature or enable extension as part of this specification.
> 
> @amaiorano is implementing this speculatively in Dawn, we are going with `immediate_address_space`, what do folks think of this name?

**General Comment** - @alan-baker (2025-11-12 15:57:15):
> > > There ought to be either a language feature or enable extension as part of this specification.
> > 
> > @amaiorano is implementing this speculatively in Dawn, we are going with `immediate_address_space`, what do folks think of this name?
> 
> That works, but I'd also be ok with just `immediates`.

**General Comment** - @Kangz (2025-11-18 17:17:58):
> <details><summary><a href="https://github.com/gpuweb/gpuweb/wiki/GPU-Web-2025%E2%80%9011%E2%80%9005#specify-immediate-data-api-and-wgsl-immediate-address-space-5423">GPU Web WG 2025-11-05 Atlantic-time</a></summary>
> 
> * Shaobo has a PR up with comments from Kai, etc  
> * 2 things to discuss  
> * 1\. Any CTS or implementation that is up to the spec yet? Shouldn't merge until we have both  
>   * JB: Moz agrees. Want CTS ready to go. Our impl isn't up to spec either.  
> * 2\. Can we make the name smaller? SetImmediates  
>   * CF: short name good 😀  
>   * JB: I have feedback on non-API-visible names, can be editorial  
> * CW: keep the PR open until we have impls / CTS ready
> 
> </details>

**General Comment** - @shaoboyan091 (2025-12-01 03:16:13):
> Rebase and iterate the PR. The new changes:
> - Adding words in wgsl `resource` part.
> - Instead of adding "padding", defines "AccessibleSlots" in wgsl spec.

**General Comment** - @dneto0 (2025-12-10 23:19:46):
> WGSL part LGTM now

**General Comment** - @kainino0x (2025-12-16 04:57:43):
> Marking draft just since we said we would finish CTS before landing this.

**General Comment** - @Kangz (2026-01-15 07:54:05):
> <details><summary><a href="https://github.com/gpuweb/gpuweb/wiki/GPU-Web-2026%E2%80%9001%E2%80%9014#specify-immediate-data-api-and-wgsl-immediate-address-space-5423">GPU Web WG 2026-01-14 Atlantic-time</a></summary>
> 
> * Getting spec / implementation complete for 1 impl of immediates  
> * What would this group want before we can merge the spec PR? On Chromium developer side, would want to be CTS and impl complete.  
> * CF: also converted wgpu's impl of push constants to the new API and everything looks good.  
> * JB: that's good. Rest in Firefox is boilerplate. Was just waiting for CTS.  
> * GT: is this a standard or optional feature?  
> * CW: core feature.  
> * GT: Stephen found it's difficult to implement in GL, for Compat. Given that this is core, if you're going to implement Compat, have to implement this.  
> * SW: if this isn't the case, we'd have to say this isn't supported in Compat.  
> * CW: Immediates is a browser feature. Browsers expose it everywhere \- Core, Compat. It isn't optional. That was the intent.  
> * SW: Connor mentioned it's implemented in GL in wgpu. It's implementable. Would have been nice if the investigation had included GL. Going forward, would be nice if stuff that impacts Compat would consider the GL backend.  
> * CW: agreed. For this PR: finish impl and CTS, and if no new discovery, merge.
> 
> </details>


## Inline Review Comments

### File: `spec/index.bs`
**[spec/index.bs:1861]** - @alan-baker (2025-11-01 01:06:34):
> It would be good to mirror this into WGSL in some form similar to workgroup variable size limit.

**[spec/index.bs:10969]** - @Kangz (2025-11-03 10:06:38):
> We didn't really discuss this in the group, but do you have any opinion about `setImmediates` instead? This is to try to improve the ergonomics very slightly but might be a bit confusing to developers given that an ArrayBuffer is passed.

**[spec/index.bs:10969]** - @shaoboyan091 (2025-11-04 15:44:21):
> I'm Ok for `setImmediates` (TBH, I'm Ok for all simplification names). But better go through community meeting.

**[spec/index.bs:10969]** - @mwyrzykowski (2025-11-05 17:00:30):
> `setImmediates` sounds good 👍

**[spec/index.bs:10970]** - @Kangz (2025-11-07 13:33:07):
> To match the way `writeBuffer` passes a range of data from the BufferSource (even if we will never pass more than 2^32 there)
> 
> ```suggestion
>         optional GPUSize64 dataOffset = 0, optional GPUSize64 size);
> ```

**[spec/index.bs:10989]** - @Copilot (2025-10-29 08:52:19):
> The type definition specifies `[=byte sequence=]` as the map value type, but at line 11410 the implementation sets the value to a `[=list=]` of 4 bytes. These types should be consistent. The definition should be changed to `[=ordered map=]<{{GPUSize32}}, [=list=]<[=byte=]>>` to match the implementation.
> ```suggestion
>     : <dfn>\[[immediate_data]]</dfn>, of type [=ordered map=]&lt;{{GPUSize32}}, [=list=]&lt;[=byte=]&gt;&gt;, initially empty
> ```

**[spec/index.bs:10989]** - @kainino0x (2025-10-31 06:06:33):
> nit: The array of arrays of bytes is a little confusing to read, I think it would be easier to describe if we tracked the data and the initialization-state separately. Data as an array of N bytes, initialization state as an array of N/4 booleans.

**[spec/index.bs:11270]** - @mwyrzykowski (2025-11-12 17:16:03):
> Is the GPUSize32 -> GPUSize64 for consistency with other buffer sizes in WebGPU? Seems reasonable 👍

**[spec/index.bs:11278]** - @Copilot (2025-10-29 08:52:19):
> The early return `return true` appears inside the validusage block (after line 11272 'If any of the following conditions are unsatisfied, return false:'). This placement is incorrect as it treats 'If |immediateSize| is 0, return true' as a validation condition rather than an early exit. This line should be moved outside the validusage block, between lines 11277 and 11279, as a separate step: '1. If |immediateSize| is 0, return `true`.'.
> ```suggestion
>     1. If |immediateSize| is 0, return `true`.
>     2. If any of the following conditions are unsatisfied, return `false`:
> 
>         <div class=validusage>
>             - |pipeline| must not be `null`.
>             - Let |pipelineLayout| be |pipeline|.{{GPUPipelineBase/[[layout]]}}.
>             - Let |immediateSize| be |pipelineLayout|.{{GPUPipelineLayout/[[immediateSize]]}}.
> ```

**[spec/index.bs:11285]** - @Copilot (2025-12-01 03:19:38):
> Typo: "initally" should be "initially".
> ```suggestion
>     : <dfn>\[[dynamic_offsets]]</dfn>, of type [=ordered map=]&lt;{{GPUIndex32}}, [=list=]&lt;{{GPUBufferDynamicOffset}}&gt;&gt;, initially empty
> ```

**[spec/index.bs:11293]** - @Copilot (2025-12-01 03:27:08):
> The `[[immediate_data]]` internal slot is described as "initially empty" but also states "The length equals the device's {{supported limits/maxImmediateSize}}." These two statements are contradictory. It should be initialized to a byte sequence of length `maxImmediateSize` with all bytes set to a default value (e.g., 0), not "initially empty".

**[spec/index.bs:11299]** - @Copilot (2025-12-01 03:27:09):
> The `[[immediate_data_set]]` internal slot is described as "initially empty" but also states "The length equals the device's {{supported limits/maxImmediateSize}} divided by 4." These two statements are contradictory. It should be initialized to a list of length `maxImmediateSize / 4` with all entries set to `false`, not "initially empty".

**[spec/index.bs:11395]** - @Kangz (2025-11-27 08:51:48):
> I believe that rangeOffset being a multiple of 4 should be a device timeline check since it is not required to implement the content side validation of the ArrayBuffer.

**[spec/index.bs:11593]** - @Kangz (2025-12-05 16:40:25):
> Since we are validating immediates at draw-time. What's the use of `GPUPipelineDescriptor.immediateSize`.

**[spec/index.bs:11610]** - @Copilot (2025-12-01 03:19:37):
> The variable `|immediateSize|` is used before it's defined. It should be defined before the check on line 11604, or the check should be moved after line 11610 where `|immediateSize|` is actually defined.
> ```suggestion
>     1. Let |pipelineLayout| be |pipeline|.{{GPUPipelineBase/[[layout]]}}.
>     2. Let |immediateSize| be |pipelineLayout|.{{GPUPipelineLayout/[[immediateSize]]}}.
>     3. If |immediateSize| is 0, return `true`.
>     4. If any of the following conditions are unsatisfied, return `false`:
> 
>         <div class=validusage>
>             - |pipeline| must not be `null`.
> ```

**[spec/index.bs:11611]** - @kainino0x (2025-10-31 06:16:18):
> When I read this I think "immediate data variable" means one `var<immediate>`, of which there is of course only one (at most). But that `var` itself has a `SizeOf` the entire struct, whereas we actually want only *accessible* parts of the struct to need to be initialized. That is going to require the WGSL spec to "reflect" some new information for us - the map of `slot -> is_accessible` - for us to validate against.
> 
> I would recommend combining this PR with the WGSL PR, since there will be new integration between the specs.

**[spec/index.bs:11611]** - @shaoboyan091 (2025-10-31 06:22:08):
> Ok, Let me do this and abandon wgsl PR.

**[spec/index.bs:11611]** - @dneto0 (2025-11-04 15:54:24):
> Unfortunately the WGSL spec doesn't really defined "padding" or "contains actual data".  It probably should, but that's probably too much to ask to fix in this PR.
> 
> I think the text in this PR is good enough for all but the most adversarial reader.  I recommend a followup PR (by the WGSL editors) to more carefully define padding and non-padding (or whatever we cal it).

**[spec/index.bs:13886]** - @Copilot (2025-12-01 03:19:38):
> The `[[immediate_data]]` property is defined as a byte sequence (line 11289), not a map. Using `[=map/Clear=]` on it is incorrect. Instead, this should likely reset all bytes to a default value or clear the `[[immediate_data_set]]` list to mark all slots as unset.
> ```suggestion
>         1. Clear |encoder|.{{GPUBindingCommandsMixin/[[immediate_data_set]]}}.
> ```


### File: `wgsl/index.bs`
**[wgsl/index.bs:4865]** - @Copilot (2025-12-01 03:19:38):
> The effective-value-type for `var<immediate>` is missing the "[=type/concrete|Concrete=]" qualifier that is present in similar entries like uniform buffers (line 4857) and storage buffers (lines 4840, 4849). For consistency and clarity, it should likely be "[=type/concrete|Concrete=] [=constructible=] [=host-shareable=], excluding arrays and structures containing array members".
> ```suggestion
>     <td>[=type/concrete|Concrete=] [=constructible=] [=host-shareable=], excluding arrays and structures containing array members
> ```

**[wgsl/index.bs:5152]** - @alan-baker (2025-11-12 03:20:28):
> As far as I can tell there could be nested structs. If we want to go with this algorithm, maybe we should instead recursively find all scalar and vector types and require the range [offset, offset + size) be set in the API?
> 
> As it stands for the structure cases I could have:
> ```
> struct S {
>   a : vec4u,
>   b : u32
> }
> struct T {
>   x : S,
>   y : u32,
> }
> var<immediate> imm : T;
> ```
> 
> Since the align of `S` is 16, `SizeOf(S)` is 32. That means some padding bytes are required to be set.
> 
> The spec uses, but does not define padding bytes. I wonder if it would be better to just define padding bytes and use that here.

**[wgsl/index.bs:7562]** - @shaoboyan091 (2025-12-01 03:12:43):
> Caught the tailing whitespace

**[wgsl/index.bs:10195]** - @alan-baker (2025-12-04 14:50:06):
> These edits look inconsistent with Shader Interfaces now. Also the carve outs for immediate data aren't great with the rest of the spec. I wonder if we should instead introduce a new interface for immediates. That might make all this cleaner. Thoughts @dneto0?

**[wgsl/index.bs:10409]** - @kainino0x (2025-10-31 23:01:08):
> Copying Alan's comment here:
> https://github.com/gpuweb/gpuweb/pull/5424#discussion_r2473448012

**[wgsl/index.bs:10409]** - @dneto0 (2025-11-04 15:34:14):
> I agree with Alan here, the "Concrete ..." part is redundant, and should be removed.

**[wgsl/index.bs:10569]** - @Copilot (2025-12-01 03:19:39):
> Inconsistent notation: `vecR` should be `vec|R|` to match the notation used elsewhere in the specification (e.g., lines 2939, 5965, 10641). The surrounding Stride computation correctly uses `vec|R|`.
> ```suggestion
>     * If |T| is a matrix with |C| columns and |R| rows, then [=AccessibleBytes=](|T|) is the union of sets `{ k + i * Stride | k in AccessibleBytes(vec|R|) }` for `i` in `0..C-1`, where `Stride` is [=roundUp=]([=AlignOf=](vec|R|), [=SizeOf=](vec|R|)).
> ```

**[wgsl/index.bs:10570]** - @alan-baker (2025-12-04 14:38:58):
> The `|T'|` doesn't render correctly. Maybe you want `|T|'`.

**[wgsl/index.bs:10570]** - @kainino0x (2025-12-04 19:01:07):
> If `|T'|` doesn't work then I would suggest something like
> ```suggestion
>     * If |T| is an atomic type <code>atomic&lt;|U|&rt;</code>, then [=AccessibleBytes=](|T|) is [=AccessibleBytes=](|U|).
> ```

**[wgsl/index.bs:10571]** - @Copilot (2025-12-01 03:27:09):
> Inconsistent variable markup: `vecR` should be `vec|R|` to match the markup used in the same line for `[=AlignOf=](vec|R|)` and `[=SizeOf=](vec|R|)`.

**[wgsl/index.bs:10981]** - @shaoboyan091 (2025-12-01 03:13:06):
> Caught the tailing space.
