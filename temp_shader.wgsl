  struct FragmentIn {
                        @builtin(position) position: vec4f,
                @location(0) @interpolate(linear, sample) interpolatedValue: vec4f,
                    };

                    struct FragOut {
                        @location(0) out0: vec4f,
                        @location(1) out1: vec4f,
                        @location(2) out2: vec4f,
                        @location(3) out3: vec4f,
                    };

                    fn u32ToRGBAUnorm(u: u32) -> vec4f {
                        return vec4f(
                        f32((u >> 24) & 0xFF) / 255.0,
                        f32((u >> 16) & 0xFF) / 255.0,
                        f32((u >>  8) & 0xFF) / 255.0,
                        f32((u >>  0) & 0xFF) / 255.0,
                        );
                    }

                    @fragment fn fs(fin: FragmentIn) -> FragOut {
                        var f: FragOut;
                        let v = fin.position;
                        let u = bitcast<vec4u>(v);
                        f.out0 = u32ToRGBAUnorm(u[0]);
                        f.out1 = u32ToRGBAUnorm(u[1]);
                        f.out2 = u32ToRGBAUnorm(u[2]);
                        f.out3 = u32ToRGBAUnorm(u[3]);
                        _ = fin.interpolatedValue;
                        return f;
                    }