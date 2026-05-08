@group(0) @binding(0) var inputTex: texture_2d<f32>;
@group(0) @binding(1) var outputTex: texture_storage_2d<rgba8unorm, write>;

const hexSize: f32 = 25.0;

@compute @workgroup_size(8, 8)
fn main(@builtin(global_invocation_id) id: vec3<u32>) {

    // 1. GET DIMENSIONS
    let texDims = textureDimensions(outputTex);

    // 2. SAFETY BOUNDS CHECK: Prevent out-of-bounds writes (CRITICAL!)
    if (id.x >= texDims.x || id.y >= texDims.y) {
        return;
    }

    // Convert pixel coordinates to normalized coordinates based on hexSize.
    let pX = f32(id.x) / hexSize;
    let pY = f32(id.y) / hexSize;

    // Convert Cartesian coordinates to axial hexagonal grid coordinates.
    let hexX = 2.0 / 3.0 * pX;
    let hexY = -1.0 / 3.0 * pX + sqrt(3.0) / 3.0 * pY;

    // Round the axial coordinates to the nearest integer
    let rx = round(hexX);
    let ry = round(hexY);
    let rs = round(-hexX - hexY);

    let diffX = abs(rx - hexX);
    let diffY = abs(ry - hexY);
    let diffS = abs(rs + hexX + hexY);

    var hexCoordX = rx;
    var hexCoordY = ry;

    if (diffX > diffY && diffX > diffS) {
        hexCoordX = -ry - rs;
    } else if (diffY > diffS) {
        hexCoordY = -rx - rs;
    }

    // Calculate the target pixel to sample from
    let centerPixel = vec2<i32>(
        vec2<f32>(1.5 * hexCoordX, sqrt(3.0) * (0.5 * hexCoordX + hexCoordY)) * hexSize
    );

    // 3. SAFETY CLAMP: Ensure we don't sample outside the input image
    let safeCenter = clamp(centerPixel, vec2<i32>(0, 0), vec2<i32>(texDims) - vec2<i32>(1, 1));

    // Load the color from the input texture at the clamped hex center.
    let color = textureLoad(inputTex, safeCenter, 0);

    // Store the loaded color into the output texture at the current pixel.
    textureStore(outputTex, id.xy, color);
}