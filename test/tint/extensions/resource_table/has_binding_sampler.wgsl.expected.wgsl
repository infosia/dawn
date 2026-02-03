enable chromium_experimental_resource_table;

@fragment
fn fs() {
  let s = hasResource<sampler>(0);
  let c = hasResource<sampler_comparison>(1);
}
