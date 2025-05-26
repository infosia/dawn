struct sk_struct_1343689961 {
  sk_member_707328391 : i32,
  sk_member_3539585619 : f32,
}

struct sk_struct_198999028 {
  sk_member_635568776 : f32,
}

struct buf0 {
  resolution : vec2<f32>,
}

@group(0) @binding(0) var<uniform> x_10 : buf0;

var<private> gl_FragCoord : vec4<f32>;

var<private> x_GLF_color : vec4<f32>;

fn compute_value_f1_f1_(limit : ptr<function, f32>, thirty_two : ptr<function, f32>) -> f32 {
  var x_91 : f32;
  var x_91_phi : f32;
  var x_94_phi : i32;
  x_91_phi = -(0.5);
  x_94_phi = 1;
  loop {
    var x_104 : f32;
    var x_113 : f32;
    if ((gl_FragCoord.w <= -0.98071230201116976755)) {
      return 0.10000000000000000555;
    }
    var x_95 : i32;
    var x_92_phi : f32;
    x_91 = x_113;
    let x_94 : i32 = x_94_phi;
    if ((x_94 < 800)) {
    } else {
      break;
    }
    var x_112 : f32;
    var x_113_phi : f32;
    if (((x_94 % 32) == 0)) {
      x_104 = (x_91 + -(-1.0));
      x_92_phi = x_104;
    } else {
      var sk_struct_var_1338488225 : sk_struct_198999028;
      var sk_var_4041214620 = sk_struct_var_1338488225;
      let x_106 : f32 = *(thirty_two);
      x_113_phi = x_91;
      if (((f32(x_94) - (round(x_106) % floor((f32(x_94) / round(x_106))))) <= 0.01000000000000000021)) {
        x_113_phi = x_112;
      }
      x_113 = x_113_phi;
      x_92_phi = x_113;
    }
    continuing {
      x_95 = (x_94 + 1);
      x_91_phi = x_113_phi;
      x_94_phi = x_95;
    }
  }
  return x_91;
}

fn main_1() {
  var sk_struct_var_993442819 : sk_struct_1343689961;
  var c : vec3<f32>;
  var param : f32;
  var param_1 : f32;

  let x_57 : f32 = compute_value_f1_f1_(&(param), &(param_1));


  return;
}

struct main_out {
  @location(0)
  x_GLF_color_1 : vec4<f32>,
}

@fragment
fn main(@builtin(position) gl_FragCoord_param : vec4<f32>) -> main_out {
  gl_FragCoord = gl_FragCoord_param;
  main_1();
  return main_out(x_GLF_color);
}
