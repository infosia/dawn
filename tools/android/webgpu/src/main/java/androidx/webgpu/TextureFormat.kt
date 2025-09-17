package androidx.webgpu

@JvmInline
public value class TextureFormat(public val value: Int) {
    public companion object {
        public val Undefined: TextureFormat = TextureFormat(0x00000000)
        public val R8Unorm: TextureFormat = TextureFormat(0x00000001)
        public val R8Snorm: TextureFormat = TextureFormat(0x00000002)
        public val R8Uint: TextureFormat = TextureFormat(0x00000003)
        public val R8Sint: TextureFormat = TextureFormat(0x00000004)
        public val R16Unorm: TextureFormat = TextureFormat(0x00000005)
        public val R16Snorm: TextureFormat = TextureFormat(0x00000006)
        public val R16Uint: TextureFormat = TextureFormat(0x00000007)
        public val R16Sint: TextureFormat = TextureFormat(0x00000008)
        public val R16Float: TextureFormat = TextureFormat(0x00000009)
        public val RG8Unorm: TextureFormat = TextureFormat(0x0000000a)
        public val RG8Snorm: TextureFormat = TextureFormat(0x0000000b)
        public val RG8Uint: TextureFormat = TextureFormat(0x0000000c)
        public val RG8Sint: TextureFormat = TextureFormat(0x0000000d)
        public val R32Float: TextureFormat = TextureFormat(0x0000000e)
        public val R32Uint: TextureFormat = TextureFormat(0x0000000f)
        public val R32Sint: TextureFormat = TextureFormat(0x00000010)
        public val RG16Unorm: TextureFormat = TextureFormat(0x00000011)
        public val RG16Snorm: TextureFormat = TextureFormat(0x00000012)
        public val RG16Uint: TextureFormat = TextureFormat(0x00000013)
        public val RG16Sint: TextureFormat = TextureFormat(0x00000014)
        public val RG16Float: TextureFormat = TextureFormat(0x00000015)
        public val RGBA8Unorm: TextureFormat = TextureFormat(0x00000016)
        public val RGBA8UnormSrgb: TextureFormat = TextureFormat(0x00000017)
        public val RGBA8Snorm: TextureFormat = TextureFormat(0x00000018)
        public val RGBA8Uint: TextureFormat = TextureFormat(0x00000019)
        public val RGBA8Sint: TextureFormat = TextureFormat(0x0000001a)
        public val BGRA8Unorm: TextureFormat = TextureFormat(0x0000001b)
        public val BGRA8UnormSrgb: TextureFormat = TextureFormat(0x0000001c)
        public val RGB10A2Uint: TextureFormat = TextureFormat(0x0000001d)
        public val RGB10A2Unorm: TextureFormat = TextureFormat(0x0000001e)
        public val RG11B10Ufloat: TextureFormat = TextureFormat(0x0000001f)
        public val RGB9E5Ufloat: TextureFormat = TextureFormat(0x00000020)
        public val RG32Float: TextureFormat = TextureFormat(0x00000021)
        public val RG32Uint: TextureFormat = TextureFormat(0x00000022)
        public val RG32Sint: TextureFormat = TextureFormat(0x00000023)
        public val RGBA16Unorm: TextureFormat = TextureFormat(0x00000024)
        public val RGBA16Snorm: TextureFormat = TextureFormat(0x00000025)
        public val RGBA16Uint: TextureFormat = TextureFormat(0x00000026)
        public val RGBA16Sint: TextureFormat = TextureFormat(0x00000027)
        public val RGBA16Float: TextureFormat = TextureFormat(0x00000028)
        public val RGBA32Float: TextureFormat = TextureFormat(0x00000029)
        public val RGBA32Uint: TextureFormat = TextureFormat(0x0000002a)
        public val RGBA32Sint: TextureFormat = TextureFormat(0x0000002b)
        public val Stencil8: TextureFormat = TextureFormat(0x0000002c)
        public val Depth16Unorm: TextureFormat = TextureFormat(0x0000002d)
        public val Depth24Plus: TextureFormat = TextureFormat(0x0000002e)
        public val Depth24PlusStencil8: TextureFormat = TextureFormat(0x0000002f)
        public val Depth32Float: TextureFormat = TextureFormat(0x00000030)
        public val Depth32FloatStencil8: TextureFormat = TextureFormat(0x00000031)
        public val BC1RGBAUnorm: TextureFormat = TextureFormat(0x00000032)
        public val BC1RGBAUnormSrgb: TextureFormat = TextureFormat(0x00000033)
        public val BC2RGBAUnorm: TextureFormat = TextureFormat(0x00000034)
        public val BC2RGBAUnormSrgb: TextureFormat = TextureFormat(0x00000035)
        public val BC3RGBAUnorm: TextureFormat = TextureFormat(0x00000036)
        public val BC3RGBAUnormSrgb: TextureFormat = TextureFormat(0x00000037)
        public val BC4RUnorm: TextureFormat = TextureFormat(0x00000038)
        public val BC4RSnorm: TextureFormat = TextureFormat(0x00000039)
        public val BC5RGUnorm: TextureFormat = TextureFormat(0x0000003a)
        public val BC5RGSnorm: TextureFormat = TextureFormat(0x0000003b)
        public val BC6HRGBUfloat: TextureFormat = TextureFormat(0x0000003c)
        public val BC6HRGBFloat: TextureFormat = TextureFormat(0x0000003d)
        public val BC7RGBAUnorm: TextureFormat = TextureFormat(0x0000003e)
        public val BC7RGBAUnormSrgb: TextureFormat = TextureFormat(0x0000003f)
        public val ETC2RGB8Unorm: TextureFormat = TextureFormat(0x00000040)
        public val ETC2RGB8UnormSrgb: TextureFormat = TextureFormat(0x00000041)
        public val ETC2RGB8A1Unorm: TextureFormat = TextureFormat(0x00000042)
        public val ETC2RGB8A1UnormSrgb: TextureFormat = TextureFormat(0x00000043)
        public val ETC2RGBA8Unorm: TextureFormat = TextureFormat(0x00000044)
        public val ETC2RGBA8UnormSrgb: TextureFormat = TextureFormat(0x00000045)
        public val EACR11Unorm: TextureFormat = TextureFormat(0x00000046)
        public val EACR11Snorm: TextureFormat = TextureFormat(0x00000047)
        public val EACRG11Unorm: TextureFormat = TextureFormat(0x00000048)
        public val EACRG11Snorm: TextureFormat = TextureFormat(0x00000049)
        public val ASTC4x4Unorm: TextureFormat = TextureFormat(0x0000004a)
        public val ASTC4x4UnormSrgb: TextureFormat = TextureFormat(0x0000004b)
        public val ASTC5x4Unorm: TextureFormat = TextureFormat(0x0000004c)
        public val ASTC5x4UnormSrgb: TextureFormat = TextureFormat(0x0000004d)
        public val ASTC5x5Unorm: TextureFormat = TextureFormat(0x0000004e)
        public val ASTC5x5UnormSrgb: TextureFormat = TextureFormat(0x0000004f)
        public val ASTC6x5Unorm: TextureFormat = TextureFormat(0x00000050)
        public val ASTC6x5UnormSrgb: TextureFormat = TextureFormat(0x00000051)
        public val ASTC6x6Unorm: TextureFormat = TextureFormat(0x00000052)
        public val ASTC6x6UnormSrgb: TextureFormat = TextureFormat(0x00000053)
        public val ASTC8x5Unorm: TextureFormat = TextureFormat(0x00000054)
        public val ASTC8x5UnormSrgb: TextureFormat = TextureFormat(0x00000055)
        public val ASTC8x6Unorm: TextureFormat = TextureFormat(0x00000056)
        public val ASTC8x6UnormSrgb: TextureFormat = TextureFormat(0x00000057)
        public val ASTC8x8Unorm: TextureFormat = TextureFormat(0x00000058)
        public val ASTC8x8UnormSrgb: TextureFormat = TextureFormat(0x00000059)
        public val ASTC10x5Unorm: TextureFormat = TextureFormat(0x0000005a)
        public val ASTC10x5UnormSrgb: TextureFormat = TextureFormat(0x0000005b)
        public val ASTC10x6Unorm: TextureFormat = TextureFormat(0x0000005c)
        public val ASTC10x6UnormSrgb: TextureFormat = TextureFormat(0x0000005d)
        public val ASTC10x8Unorm: TextureFormat = TextureFormat(0x0000005e)
        public val ASTC10x8UnormSrgb: TextureFormat = TextureFormat(0x0000005f)
        public val ASTC10x10Unorm: TextureFormat = TextureFormat(0x00000060)
        public val ASTC10x10UnormSrgb: TextureFormat = TextureFormat(0x00000061)
        public val ASTC12x10Unorm: TextureFormat = TextureFormat(0x00000062)
        public val ASTC12x10UnormSrgb: TextureFormat = TextureFormat(0x00000063)
        public val ASTC12x12Unorm: TextureFormat = TextureFormat(0x00000064)
        public val ASTC12x12UnormSrgb: TextureFormat = TextureFormat(0x00000065)
        internal val names: Map<Int, String> = mapOf(
            0x00000000 to "Undefined",
            0x00000001 to "R8Unorm",
            0x00000002 to "R8Snorm",
            0x00000003 to "R8Uint",
            0x00000004 to "R8Sint",
            0x00000005 to "R16Unorm",
            0x00000006 to "R16Snorm",
            0x00000007 to "R16Uint",
            0x00000008 to "R16Sint",
            0x00000009 to "R16Float",
            0x0000000a to "RG8Unorm",
            0x0000000b to "RG8Snorm",
            0x0000000c to "RG8Uint",
            0x0000000d to "RG8Sint",
            0x0000000e to "R32Float",
            0x0000000f to "R32Uint",
            0x00000010 to "R32Sint",
            0x00000011 to "RG16Unorm",
            0x00000012 to "RG16Snorm",
            0x00000013 to "RG16Uint",
            0x00000014 to "RG16Sint",
            0x00000015 to "RG16Float",
            0x00000016 to "RGBA8Unorm",
            0x00000017 to "RGBA8UnormSrgb",
            0x00000018 to "RGBA8Snorm",
            0x00000019 to "RGBA8Uint",
            0x0000001a to "RGBA8Sint",
            0x0000001b to "BGRA8Unorm",
            0x0000001c to "BGRA8UnormSrgb",
            0x0000001d to "RGB10A2Uint",
            0x0000001e to "RGB10A2Unorm",
            0x0000001f to "RG11B10Ufloat",
            0x00000020 to "RGB9E5Ufloat",
            0x00000021 to "RG32Float",
            0x00000022 to "RG32Uint",
            0x00000023 to "RG32Sint",
            0x00000024 to "RGBA16Unorm",
            0x00000025 to "RGBA16Snorm",
            0x00000026 to "RGBA16Uint",
            0x00000027 to "RGBA16Sint",
            0x00000028 to "RGBA16Float",
            0x00000029 to "RGBA32Float",
            0x0000002a to "RGBA32Uint",
            0x0000002b to "RGBA32Sint",
            0x0000002c to "Stencil8",
            0x0000002d to "Depth16Unorm",
            0x0000002e to "Depth24Plus",
            0x0000002f to "Depth24PlusStencil8",
            0x00000030 to "Depth32Float",
            0x00000031 to "Depth32FloatStencil8",
            0x00000032 to "BC1RGBAUnorm",
            0x00000033 to "BC1RGBAUnormSrgb",
            0x00000034 to "BC2RGBAUnorm",
            0x00000035 to "BC2RGBAUnormSrgb",
            0x00000036 to "BC3RGBAUnorm",
            0x00000037 to "BC3RGBAUnormSrgb",
            0x00000038 to "BC4RUnorm",
            0x00000039 to "BC4RSnorm",
            0x0000003a to "BC5RGUnorm",
            0x0000003b to "BC5RGSnorm",
            0x0000003c to "BC6HRGBUfloat",
            0x0000003d to "BC6HRGBFloat",
            0x0000003e to "BC7RGBAUnorm",
            0x0000003f to "BC7RGBAUnormSrgb",
            0x00000040 to "ETC2RGB8Unorm",
            0x00000041 to "ETC2RGB8UnormSrgb",
            0x00000042 to "ETC2RGB8A1Unorm",
            0x00000043 to "ETC2RGB8A1UnormSrgb",
            0x00000044 to "ETC2RGBA8Unorm",
            0x00000045 to "ETC2RGBA8UnormSrgb",
            0x00000046 to "EACR11Unorm",
            0x00000047 to "EACR11Snorm",
            0x00000048 to "EACRG11Unorm",
            0x00000049 to "EACRG11Snorm",
            0x0000004a to "ASTC4x4Unorm",
            0x0000004b to "ASTC4x4UnormSrgb",
            0x0000004c to "ASTC5x4Unorm",
            0x0000004d to "ASTC5x4UnormSrgb",
            0x0000004e to "ASTC5x5Unorm",
            0x0000004f to "ASTC5x5UnormSrgb",
            0x00000050 to "ASTC6x5Unorm",
            0x00000051 to "ASTC6x5UnormSrgb",
            0x00000052 to "ASTC6x6Unorm",
            0x00000053 to "ASTC6x6UnormSrgb",
            0x00000054 to "ASTC8x5Unorm",
            0x00000055 to "ASTC8x5UnormSrgb",
            0x00000056 to "ASTC8x6Unorm",
            0x00000057 to "ASTC8x6UnormSrgb",
            0x00000058 to "ASTC8x8Unorm",
            0x00000059 to "ASTC8x8UnormSrgb",
            0x0000005a to "ASTC10x5Unorm",
            0x0000005b to "ASTC10x5UnormSrgb",
            0x0000005c to "ASTC10x6Unorm",
            0x0000005d to "ASTC10x6UnormSrgb",
            0x0000005e to "ASTC10x8Unorm",
            0x0000005f to "ASTC10x8UnormSrgb",
            0x00000060 to "ASTC10x10Unorm",
            0x00000061 to "ASTC10x10UnormSrgb",
            0x00000062 to "ASTC12x10Unorm",
            0x00000063 to "ASTC12x10UnormSrgb",
            0x00000064 to "ASTC12x12Unorm",
            0x00000065 to "ASTC12x12UnormSrgb",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
