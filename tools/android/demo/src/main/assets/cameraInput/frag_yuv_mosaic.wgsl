@group(0) @binding(0) var mySampler: sampler;
@group(0) @binding(1) var myVideoTexture: texture_external;

const hexSize: f32 = 25.0;

@fragment
fn main(@location(0) uv: vec2<f32>) -> @location(0) vec4<f32> {

    // 1. GET DIMENSIONS (Returns vec2<u32>, so we cast to f32)
    let texDims = vec2<f32>(textureDimensions(myVideoTexture));

    // Convert the 0.0-1.0 UV into exact pixel coordinates (e.g., 1920x1080)
    let pixelCoord = uv * texDims;

    // 2. HEXAGONAL MATH
    let pX = pixelCoord.x / hexSize;
    let pY = pixelCoord.y / hexSize;

    let hexX = 2.0 / 3.0 * pX;
    let hexY = -1.0 / 3.0 * pX + sqrt(3.0) / 3.0 * pY;

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

    // Calculate the absolute pixel coordinate of the hexagon's center
    let centerPixel = vec2<f32>(
        1.5 * hexCoordX,
        sqrt(3.0) * (0.5 * hexCoordX + hexCoordY)
    ) * hexSize;

    // 3. CONVERT BACK TO UV FOR SAMPLING
    let centerUv = centerPixel / texDims;

    // 4. SAMPLE THE EXTERNAL TEXTURE
    // The hardware magically reads the Y, U, and V bytes and converts them to RGB here!
    let color = textureSampleBaseClampToEdge(myVideoTexture, mySampler, centerUv);

    return color;
}