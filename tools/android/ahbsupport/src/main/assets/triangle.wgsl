// Define what comes IN from the Vertex Buffer
struct VertexInput{
 @location(0) position: vec4<f32>,
 @location(1) color: vec4<f32>,
};

// Define what goes OUT to the Fragment Shader
struct VertexOutput{
 @builtin(position) clip_position: vec4<f32>,
 @location(0) color : vec4<f32>,
};

@vertex
fn vs_main(model: VertexInput) -> VertexOutput{
  var out : VertexOutput;
  out.clip_position = model.position;
  out.color = model.color;

  return out;
}

@fragment
fn fs_main(model : VertexOutput)-> @location(0) vec4<f32>{
  return model.color;
}