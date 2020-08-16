#version 430

// mathematical constants
#define M_PI 3.1415926535898
#define M_PI_2 1.57079632679
#define SQRT_3 1.732050808
#define HALF_ROOT_2 0.70710676908493042

layout (local_size_x = 32, local_size_y = 32, local_size_z = 1) in;
layout (std430, binding = 1) buffer rasterBuffer { float rasters []; };            // array of input rasters for convolutions //                  if x has height 0 then it is a cubix sphere, otherwise an equirectangular raster

// this is the cubic sphere into which the output will be written
layout (std430, binding = 0) buffer heightMapBuffer { float height_map_out []; };

uniform vec4 bounds = vec4 (-M_PI, -M_PI / 2, M_PI, M_PI / 2);

// indices to faces of the cube map


#define FACE_FRONT 0
#define FACE_BACK 1
#define FACE_NORTH 2
#define FACE_SOUTH 3
#define FACE_EAST 4
#define FACE_WEST 5

// noise seeds
const int X_NOISE_GEN = 1619;
const int Y_NOISE_GEN = 31337;
const int Z_NOISE_GEN = 6971;
const int SEED_NOISE_GEN = 1013;
const int SHIFT_NOISE_GEN = 8;


// these constants are used to scale and bias the outputs of noise generators
// so that they fall in the range -1 to 1 (roughly)
const float SIMPLEX_BIAS = 0.57811;
const float SIMPLEX_SCALE = 0.62083034;
const float RIDGED_MULTI_BIAS = 0.864406;
const float RIDGED_MULTI_SCALE = 1.091014622;
const float VORONOI_BIAS = 0;
const float VORONOI_SCALE = 1.757700928;
const float PERLIN_BIAS = 0.0;
const float PERLIN_SCALE = 1.0;
const float BILLOW_BIAS = 0.0;
const float BILLOW_SCALE = 1.0;



// output identifiers
const int MODE_EQUIRECTANGULAR = 0;
const int MODE_VERTEXLIST = 3;
const int MODE_CUBEMAP = 4;

uniform int mode = MODE_EQUIRECTANGULAR;

// statistics
int hypsographyResolution;
float minAltitude = 0;
float maxAltitude = 0;

// grid parameters
uniform int vertexCount;

// cubemap parameter
uniform int size;


vec3 toCartesian (in vec2 geolocation) { // x = longitude, y = latitude
    vec3 cartesian;
    cartesian.x = cos (geolocation.x) * cos (geolocation.y);
    cartesian.z = cos (geolocation.y) * sin (geolocation.x);
    cartesian.y = sin (geolocation.y);
    return cartesian;
}

vec2 toGeolocation (vec3 cartesian) {
    float phi = atan (cartesian.z, cartesian.x);
    float theta = acos (cartesian.y) - (M_PI / 2);
    return vec2 (phi, -theta);
}

float hash (float n) {
    return fract (sin (n) * 43758.5453);
}


// Cubic spline: y = x^2 (3 - 2x) = 3x^2 - 2x^3
float SCurve3 (float x) {
    return (x * x * (3.0 - 2.0 * x));
}

// Quintic spline: y = 6x^5 - 15x^4 + 10x^3
float SCurve5 (float x) {
    float x3 = x * x * x;
    float x4 = x3 * x;
    float x5 = x4 * x;
    return (6.0 * x5) - (15.0 * x4) + (10.0 * x3);
}

/// Cubic interpolation with four controls (from libnoise).
/// The alpha value should range from 0.0 to 1.0.  If the alpha value is
/// 0.0, this function returns @a n1.  If the alpha value is 1.0, this
/// function returns @a n2.
float cubicInterpolate (float n0, float n1, float n2, float n3, float a) {
    float p = (n3 - n2) - (n0 - n1);
    float q = (n0 - n1) - p;
    float r = n2 - n0;
    float s = n1;
    return p * a * a * a + q * a * a + r * a + s;
}

vec3 rotate (float theta, vec3 pole, vec3 c) {
    vec4 quat = vec4 (cos (theta / 2), sin (theta / 2) * pole.x, sin (theta / 2) * pole.y, sin (theta / 2) * pole.z);
    float w = quat.x;
    float x = quat.y;
    float y = quat.z;
    float z = quat.w;

    mat3 m = mat3  (1 - 2 * y * y - 2 * z * z, 2 * x * y - 2 * w * z, 2 * x * z + 2 * w * y, // 0.0,
                    2 * x * y + 2 * w * z, 1 - 2 * x * x - 2 * z * z, 2 * y * z - 2 * w * x, //0.0,
                    2 * x * z - 2 * w * y, 2 * y * z + 2 * w * x, 1 - 2 * x * x - 2 * y * y//, 0.0,
                    //0.0, 0.0, 0.0, 1.0
                    );

    return vec3 (m [0].x * c.x + m [0].y * c.y + m [0].z * c.z,
                 m [1].x * c.x + m [1].y * c.y + m [1].z * c.z,
                 m [2].x * c.x + m [2].y * c.y + m [2].z * c.z);
}


//	Classic Perlin 4D Noise
//	by Stefan Gustavson
//
vec4 permute(vec4 x){return mod(((x*34.0)+1.0)*x, 289.0);}
vec4 taylorInvSqrt(vec4 r){return 1.79284291400159 - 0.85373472095314 * r;}
vec4 fade(vec4 t) {return t*t*t*(t*(t*6.0-15.0)+10.0);}

float cnoise (vec4 P){
    vec4 Pi0 = floor(P); // Integer part for indexing
    vec4 Pi1 = Pi0 + 1.0; // Integer part + 1
    Pi0 = mod(Pi0, 289.0);
    Pi1 = mod(Pi1, 289.0);
    vec4 Pf0 = fract(P); // Fractional part for interpolation
    vec4 Pf1 = Pf0 - 1.0; // Fractional part - 1.0
    vec4 ix = vec4(Pi0.x, Pi1.x, Pi0.x, Pi1.x);
    vec4 iy = vec4(Pi0.yy, Pi1.yy);
    vec4 iz0 = vec4(Pi0.zzzz);
    vec4 iz1 = vec4(Pi1.zzzz);
    vec4 iw0 = vec4(Pi0.wwww);
    vec4 iw1 = vec4(Pi1.wwww);

    vec4 ixy = permute(permute(ix) + iy);
    vec4 ixy0 = permute(ixy + iz0);
    vec4 ixy1 = permute(ixy + iz1);
    vec4 ixy00 = permute(ixy0 + iw0);
    vec4 ixy01 = permute(ixy0 + iw1);
    vec4 ixy10 = permute(ixy1 + iw0);
    vec4 ixy11 = permute(ixy1 + iw1);

    vec4 gx00 = ixy00 / 7.0;
    vec4 gy00 = floor(gx00) / 7.0;
    vec4 gz00 = floor(gy00) / 6.0;
    gx00 = fract(gx00) - 0.5;
    gy00 = fract(gy00) - 0.5;
    gz00 = fract(gz00) - 0.5;
    vec4 gw00 = vec4(0.75) - abs(gx00) - abs(gy00) - abs(gz00);
    vec4 sw00 = step(gw00, vec4(0.0));
    gx00 -= sw00 * (step(0.0, gx00) - 0.5);
    gy00 -= sw00 * (step(0.0, gy00) - 0.5);

    vec4 gx01 = ixy01 / 7.0;
    vec4 gy01 = floor(gx01) / 7.0;
    vec4 gz01 = floor(gy01) / 6.0;
    gx01 = fract(gx01) - 0.5;
    gy01 = fract(gy01) - 0.5;
    gz01 = fract(gz01) - 0.5;
    vec4 gw01 = vec4(0.75) - abs(gx01) - abs(gy01) - abs(gz01);
    vec4 sw01 = step(gw01, vec4(0.0));
    gx01 -= sw01 * (step(0.0, gx01) - 0.5);
    gy01 -= sw01 * (step(0.0, gy01) - 0.5);

    vec4 gx10 = ixy10 / 7.0;
    vec4 gy10 = floor(gx10) / 7.0;
    vec4 gz10 = floor(gy10) / 6.0;
    gx10 = fract(gx10) - 0.5;
    gy10 = fract(gy10) - 0.5;
    gz10 = fract(gz10) - 0.5;
    vec4 gw10 = vec4(0.75) - abs(gx10) - abs(gy10) - abs(gz10);
    vec4 sw10 = step(gw10, vec4(0.0));
    gx10 -= sw10 * (step(0.0, gx10) - 0.5);
    gy10 -= sw10 * (step(0.0, gy10) - 0.5);

    vec4 gx11 = ixy11 / 7.0;
    vec4 gy11 = floor(gx11) / 7.0;
    vec4 gz11 = floor(gy11) / 6.0;
    gx11 = fract(gx11) - 0.5;
    gy11 = fract(gy11) - 0.5;
    gz11 = fract(gz11) - 0.5;
    vec4 gw11 = vec4(0.75) - abs(gx11) - abs(gy11) - abs(gz11);
    vec4 sw11 = step(gw11, vec4(0.0));
    gx11 -= sw11 * (step(0.0, gx11) - 0.5);
    gy11 -= sw11 * (step(0.0, gy11) - 0.5);

    vec4 g0000 = vec4(gx00.x,gy00.x,gz00.x,gw00.x);
    vec4 g1000 = vec4(gx00.y,gy00.y,gz00.y,gw00.y);
    vec4 g0100 = vec4(gx00.z,gy00.z,gz00.z,gw00.z);
    vec4 g1100 = vec4(gx00.w,gy00.w,gz00.w,gw00.w);
    vec4 g0010 = vec4(gx10.x,gy10.x,gz10.x,gw10.x);
    vec4 g1010 = vec4(gx10.y,gy10.y,gz10.y,gw10.y);
    vec4 g0110 = vec4(gx10.z,gy10.z,gz10.z,gw10.z);
    vec4 g1110 = vec4(gx10.w,gy10.w,gz10.w,gw10.w);
    vec4 g0001 = vec4(gx01.x,gy01.x,gz01.x,gw01.x);
    vec4 g1001 = vec4(gx01.y,gy01.y,gz01.y,gw01.y);
    vec4 g0101 = vec4(gx01.z,gy01.z,gz01.z,gw01.z);
    vec4 g1101 = vec4(gx01.w,gy01.w,gz01.w,gw01.w);
    vec4 g0011 = vec4(gx11.x,gy11.x,gz11.x,gw11.x);
    vec4 g1011 = vec4(gx11.y,gy11.y,gz11.y,gw11.y);
    vec4 g0111 = vec4(gx11.z,gy11.z,gz11.z,gw11.z);
    vec4 g1111 = vec4(gx11.w,gy11.w,gz11.w,gw11.w);

    vec4 norm00 = taylorInvSqrt(vec4(dot(g0000, g0000), dot(g0100, g0100), dot(g1000, g1000), dot(g1100, g1100)));
    g0000 *= norm00.x;
    g0100 *= norm00.y;
    g1000 *= norm00.z;
    g1100 *= norm00.w;

    vec4 norm01 = taylorInvSqrt(vec4(dot(g0001, g0001), dot(g0101, g0101), dot(g1001, g1001), dot(g1101, g1101)));
    g0001 *= norm01.x;
    g0101 *= norm01.y;
    g1001 *= norm01.z;
    g1101 *= norm01.w;

    vec4 norm10 = taylorInvSqrt(vec4(dot(g0010, g0010), dot(g0110, g0110), dot(g1010, g1010), dot(g1110, g1110)));
    g0010 *= norm10.x;
    g0110 *= norm10.y;
    g1010 *= norm10.z;
    g1110 *= norm10.w;

    vec4 norm11 = taylorInvSqrt(vec4(dot(g0011, g0011), dot(g0111, g0111), dot(g1011, g1011), dot(g1111, g1111)));
    g0011 *= norm11.x;
    g0111 *= norm11.y;
    g1011 *= norm11.z;
    g1111 *= norm11.w;

    float n0000 = dot(g0000, Pf0);
    float n1000 = dot(g1000, vec4(Pf1.x, Pf0.yzw));
    float n0100 = dot(g0100, vec4(Pf0.x, Pf1.y, Pf0.zw));
    float n1100 = dot(g1100, vec4(Pf1.xy, Pf0.zw));
    float n0010 = dot(g0010, vec4(Pf0.xy, Pf1.z, Pf0.w));
    float n1010 = dot(g1010, vec4(Pf1.x, Pf0.y, Pf1.z, Pf0.w));
    float n0110 = dot(g0110, vec4(Pf0.x, Pf1.yz, Pf0.w));
    float n1110 = dot(g1110, vec4(Pf1.xyz, Pf0.w));
    float n0001 = dot(g0001, vec4(Pf0.xyz, Pf1.w));
    float n1001 = dot(g1001, vec4(Pf1.x, Pf0.yz, Pf1.w));
    float n0101 = dot(g0101, vec4(Pf0.x, Pf1.y, Pf0.z, Pf1.w));
    float n1101 = dot(g1101, vec4(Pf1.xy, Pf0.z, Pf1.w));
    float n0011 = dot(g0011, vec4(Pf0.xy, Pf1.zw));
    float n1011 = dot(g1011, vec4(Pf1.x, Pf0.y, Pf1.zw));
    float n0111 = dot(g0111, vec4(Pf0.x, Pf1.yzw));
    float n1111 = dot(g1111, Pf1);

    vec4 fade_xyzw = fade(Pf0);
    vec4 n_0w = mix(vec4(n0000, n1000, n0100, n1100), vec4(n0001, n1001, n0101, n1101), fade_xyzw.w);
    vec4 n_1w = mix(vec4(n0010, n1010, n0110, n1110), vec4(n0011, n1011, n0111, n1111), fade_xyzw.w);
    vec4 n_zw = mix(n_0w, n_1w, fade_xyzw.z);
    vec2 n_yzw = mix(n_zw.xy, n_zw.zw, fade_xyzw.y);
    float n_xyzw = mix(n_yzw.x, n_yzw.y, fade_xyzw.x);
    return 2.2 * n_xyzw;
}

//	Simplex 4D Noise
//	by Ian McEwan, Ashima Arts
//
float permute (float x) { return floor (mod (((x*34.0)+1.0)*x, 289.0)); }
float taylorInvSqrt (float r) { return 1.79284291400159 - 0.85373472095314 * r; }


vec4 grad4 (float j, vec4 ip){
    const vec4 ones = vec4(1.0, 1.0, 1.0, -1.0);
    vec4 p,s;

    p.xyz = floor (fract (vec3 (j) * ip.xyz) * 7.0) * ip.z - 1.0;
    p.w = 1.5 - dot (abs (p.xyz), ones.xyz);
    s = vec4 (lessThan (p, vec4(0.0)));
    p.xyz = p.xyz + (s.xyz * 2.0 - 1.0) * s.www;

    return p;
}

float snoise (vec4 v){
    const vec2  C = vec2 (0.138196601125010504,  // (5 - sqrt(5))/20  G4
    0.309016994374947451); // (sqrt(5) - 1)/4   F4
    // First corner
    vec4 i  = floor(v + dot(v, C.yyyy) );
    vec4 x0 = v -   i + dot(i, C.xxxx);

    // Other corners

    // Rank sorting originally contributed by Bill Licea-Kane, AMD (formerly ATI)
    vec4 i0;

    vec3 isX = step( x0.yzw, x0.xxx );
    vec3 isYZ = step( x0.zww, x0.yyz );
    //  i0.x = dot( isX, vec3( 1.0 ) );
    i0.x = isX.x + isX.y + isX.z;
    i0.yzw = 1.0 - isX;

    //  i0.y += dot( isYZ.xy, vec2( 1.0 ) );
    i0.y += isYZ.x + isYZ.y;
    i0.zw += 1.0 - isYZ.xy;

    i0.z += isYZ.z;
    i0.w += 1.0 - isYZ.z;

    // i0 now contains the unique values 0,1,2,3 in each channel
    vec4 i3 = clamp( i0, 0.0, 1.0 );
    vec4 i2 = clamp( i0-1.0, 0.0, 1.0 );
    vec4 i1 = clamp( i0-2.0, 0.0, 1.0 );

    //  x0 = x0 - 0.0 + 0.0 * C
    vec4 x1 = x0 - i1 + 1.0 * C.xxxx;
    vec4 x2 = x0 - i2 + 2.0 * C.xxxx;
    vec4 x3 = x0 - i3 + 3.0 * C.xxxx;
    vec4 x4 = x0 - 1.0 + 4.0 * C.xxxx;

    // Permutations
    i = mod(i, 289.0);
    float j0 = permute( permute( permute( permute(i.w) + i.z) + i.y) + i.x);
    vec4 j1 = permute( permute( permute( permute (
    i.w + vec4(i1.w, i2.w, i3.w, 1.0 ))
    + i.z + vec4(i1.z, i2.z, i3.z, 1.0 ))
    + i.y + vec4(i1.y, i2.y, i3.y, 1.0 ))
    + i.x + vec4(i1.x, i2.x, i3.x, 1.0 ));
    // Gradients
    // ( 7*7*6 points uniformly over a cube, mapped onto a 4-octahedron.)
    // 7*7*6 = 294, which is close to the ring size 17*17 = 289.

    vec4 ip = vec4(1.0/294.0, 1.0/49.0, 1.0/7.0, 0.0) ;

    vec4 p0 = grad4(j0,   ip);
    vec4 p1 = grad4(j1.x, ip);
    vec4 p2 = grad4(j1.y, ip);
    vec4 p3 = grad4(j1.z, ip);
    vec4 p4 = grad4(j1.w, ip);

    // Normalise gradients
    vec4 norm = taylorInvSqrt(vec4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));
    p0 *= norm.x;
    p1 *= norm.y;
    p2 *= norm.z;
    p3 *= norm.w;
    p4 *= taylorInvSqrt(dot(p4,p4));

    // Mix contributions from the five corners
    vec3 m0 = max(0.6 - vec3(dot(x0,x0), dot(x1,x1), dot(x2,x2)), 0.0);
    vec2 m1 = max(0.6 - vec2(dot(x3,x3), dot(x4,x4)            ), 0.0);
    m0 = m0 * m0;
    m1 = m1 * m1;
    return 49.0 * ( dot(m0*m0, vec3( dot( p0, x0 ), dot( p1, x1 ), dot( p2, x2 )))
    + dot(m1*m1, vec2( dot( p3, x3 ), dot( p4, x4 ) ) ) ) ;

}

// Cellular noise ("Worley noise") in 3D in GLSL.
// Copyright (c) Stefan Gustavson 2011-04-19. All rights reserved.
// This code is released under the conditions of the MIT license.
// See LICENSE file for details.

// Permutation polynomial: (34x^2 + x) mod 289
vec3 permute (vec3 x) {
    return mod((34.0 * x + 1.0) * x, 289.0);
}

// Cellular noise, returning F1 and F2 in a vec2.
// 3x3x3 search region for good F2 everywhere, but a lot
// slower than the 2x2x2 version.
// The code below is a bit scary even to its author,
// but it has at least half decent performance on a
// modern GPU. In any case, it beats any software
// implementation of Worley noise hands down.

vec2 cellular (vec3 P, float jitter, float seed) {
    P += seed;
    #define K 0.142857142857 // 1/7
    #define Ko 0.428571428571 // 1/2-K/2
    #define K2 0.020408163265306 // 1/(7*7)
    #define Kz 0.166666666667 // 1/6
    #define Kzo 0.416666666667 // 1/2-1/6*2


    vec3 Pi = mod(floor(P), 289.0);
    vec3 Pf = fract(P) - 0.5;

    vec3 Pfx = Pf.x + vec3(1.0, 0.0, -1.0);
    vec3 Pfy = Pf.y + vec3(1.0, 0.0, -1.0);
    vec3 Pfz = Pf.z + vec3(1.0, 0.0, -1.0);

    vec3 p = permute(Pi.x + vec3(-1.0, 0.0, 1.0));
    vec3 p1 = permute(p + Pi.y - 1.0);
    vec3 p2 = permute(p + Pi.y);
    vec3 p3 = permute(p + Pi.y + 1.0);

    vec3 p11 = permute(p1 + Pi.z - 1.0);
    vec3 p12 = permute(p1 + Pi.z);
    vec3 p13 = permute(p1 + Pi.z + 1.0);

    vec3 p21 = permute(p2 + Pi.z - 1.0);
    vec3 p22 = permute(p2 + Pi.z);
    vec3 p23 = permute(p2 + Pi.z + 1.0);

    vec3 p31 = permute(p3 + Pi.z - 1.0);
    vec3 p32 = permute(p3 + Pi.z);
    vec3 p33 = permute(p3 + Pi.z + 1.0);

    vec3 ox11 = fract(p11*K) - Ko;
    vec3 oy11 = mod(floor(p11*K), 7.0)*K - Ko;
    vec3 oz11 = floor(p11*K2)*Kz - Kzo; // p11 < 289 guaranteed

    vec3 ox12 = fract(p12*K) - Ko;
    vec3 oy12 = mod(floor(p12*K), 7.0)*K - Ko;
    vec3 oz12 = floor(p12*K2)*Kz - Kzo;

    vec3 ox13 = fract(p13*K) - Ko;
    vec3 oy13 = mod(floor(p13*K), 7.0)*K - Ko;
    vec3 oz13 = floor(p13*K2)*Kz - Kzo;

    vec3 ox21 = fract(p21*K) - Ko;
    vec3 oy21 = mod(floor(p21*K), 7.0)*K - Ko;
    vec3 oz21 = floor(p21*K2)*Kz - Kzo;

    vec3 ox22 = fract(p22*K) - Ko;
    vec3 oy22 = mod(floor(p22*K), 7.0)*K - Ko;
    vec3 oz22 = floor(p22*K2)*Kz - Kzo;

    vec3 ox23 = fract(p23*K) - Ko;
    vec3 oy23 = mod(floor(p23*K), 7.0)*K - Ko;
    vec3 oz23 = floor(p23*K2)*Kz - Kzo;

    vec3 ox31 = fract(p31*K) - Ko;
    vec3 oy31 = mod(floor(p31*K), 7.0)*K - Ko;
    vec3 oz31 = floor(p31*K2)*Kz - Kzo;

    vec3 ox32 = fract(p32*K) - Ko;
    vec3 oy32 = mod(floor(p32*K), 7.0)*K - Ko;
    vec3 oz32 = floor(p32*K2)*Kz - Kzo;

    vec3 ox33 = fract(p33*K) - Ko;
    vec3 oy33 = mod(floor(p33*K), 7.0)*K - Ko;
    vec3 oz33 = floor(p33*K2)*Kz - Kzo;

    vec3 dx11 = Pfx + jitter*ox11;
    vec3 dy11 = Pfy.x + jitter*oy11;
    vec3 dz11 = Pfz.x + jitter*oz11;

    vec3 dx12 = Pfx + jitter*ox12;
    vec3 dy12 = Pfy.x + jitter*oy12;
    vec3 dz12 = Pfz.y + jitter*oz12;

    vec3 dx13 = Pfx + jitter*ox13;
    vec3 dy13 = Pfy.x + jitter*oy13;
    vec3 dz13 = Pfz.z + jitter*oz13;

    vec3 dx21 = Pfx + jitter*ox21;
    vec3 dy21 = Pfy.y + jitter*oy21;
    vec3 dz21 = Pfz.x + jitter*oz21;

    vec3 dx22 = Pfx + jitter*ox22;
    vec3 dy22 = Pfy.y + jitter*oy22;
    vec3 dz22 = Pfz.y + jitter*oz22;

    vec3 dx23 = Pfx + jitter*ox23;
    vec3 dy23 = Pfy.y + jitter*oy23;
    vec3 dz23 = Pfz.z + jitter*oz23;

    vec3 dx31 = Pfx + jitter*ox31;
    vec3 dy31 = Pfy.z + jitter*oy31;
    vec3 dz31 = Pfz.x + jitter*oz31;

    vec3 dx32 = Pfx + jitter*ox32;
    vec3 dy32 = Pfy.z + jitter*oy32;
    vec3 dz32 = Pfz.y + jitter*oz32;

    vec3 dx33 = Pfx + jitter*ox33;
    vec3 dy33 = Pfy.z + jitter*oy33;
    vec3 dz33 = Pfz.z + jitter*oz33;

    vec3 d11 = dx11 * dx11 + dy11 * dy11 + dz11 * dz11;
    vec3 d12 = dx12 * dx12 + dy12 * dy12 + dz12 * dz12;
    vec3 d13 = dx13 * dx13 + dy13 * dy13 + dz13 * dz13;
    vec3 d21 = dx21 * dx21 + dy21 * dy21 + dz21 * dz21;
    vec3 d22 = dx22 * dx22 + dy22 * dy22 + dz22 * dz22;
    vec3 d23 = dx23 * dx23 + dy23 * dy23 + dz23 * dz23;
    vec3 d31 = dx31 * dx31 + dy31 * dy31 + dz31 * dz31;
    vec3 d32 = dx32 * dx32 + dy32 * dy32 + dz32 * dz32;
    vec3 d33 = dx33 * dx33 + dy33 * dy33 + dz33 * dz33;

    // Do it right and sort out both F1 and F2
    vec3 d1a = min(d11, d12);
    d12 = max(d11, d12);
    d11 = min(d1a, d13); // Smallest now not in d12 or d13
    d13 = max(d1a, d13);
    d12 = min(d12, d13); // 2nd smallest now not in d13
    vec3 d2a = min(d21, d22);
    d22 = max(d21, d22);
    d21 = min(d2a, d23); // Smallest now not in d22 or d23
    d23 = max(d2a, d23);
    d22 = min(d22, d23); // 2nd smallest now not in d23
    vec3 d3a = min(d31, d32);
    d32 = max(d31, d32);
    d31 = min(d3a, d33); // Smallest now not in d32 or d33
    d33 = max(d3a, d33);
    d32 = min(d32, d33); // 2nd smallest now not in d33
    vec3 da = min(d11, d21);
    d21 = max(d11, d21);
    d11 = min(da, d31); // Smallest now in d11
    d31 = max(da, d31); // 2nd smallest now not in d31
    d11.xy = (d11.x < d11.y) ? d11.xy : d11.yx;
    d11.xz = (d11.x < d11.z) ? d11.xz : d11.zx; // d11.x now smallest
    d12 = min(d12, d21); // 2nd smallest now not in d21
    d12 = min(d12, d22); // nor in d22
    d12 = min(d12, d31); // nor in d31
    d12 = min(d12, d32); // nor in d32
    d11.yz = min(d11.yz,d12.xy); // nor in d12.yz
    d11.y = min(d11.y,d12.z); // Only two more to go
    d11.y = min(d11.y,d11.z); // Done! (Phew!)
    return sqrt(d11.xy); // F1, F2
}

float voronoi (vec3 cartesian, float frequency, float displacement, float voronoiScale, int seed) {
    vec2 c = cellular (cartesian * frequency, displacement, seed);
    return ((((c.y - c.x) + VORONOI_BIAS) * VORONOI_SCALE) - 1.0) * voronoiScale;

}

float noise (vec3 cartesian, bool simplex, float frequency, float lacunarity, float persistence, int octaves, int seed) {
    float value = 0.0;
    float signal = 0.0;
    float curPersistence = 1.0;
    vec3 n = cartesian; // vec3 (makeInt32Range (cartesian.x), makeInt32Range (cartesian.y), makeInt32Range (cartesian.z));

    n *= frequency;

    for (int curOctave = 0; curOctave < octaves; curOctave++) {
        seed = (seed + curOctave) & 0xffffffff;
        //signal = simplex ? snoise (vec4 (n.xyz, seed)) : cnoise (vec4 (n.xyz, seed));
        signal = snoise (vec4 (n.xyz, seed));
        value += signal * curPersistence;

        // Prepare the next octave.
        n *= lacunarity;
        curPersistence *= persistence;
    }

    return value;
}

float perlin (vec3 cartesian, float frequency, float lacunarity, float persistence, int octaves, int seed) {
    // rewired to use simplex noise for now as faster - maybe no need to implement perlin
    return (noise (cartesian, true, frequency, lacunarity, persistence, octaves, seed) + PERLIN_BIAS) * PERLIN_SCALE;
}

float simplex (vec3 cartesian, float frequency, float lacunarity, float persistence, int octaves, int seed) {
    return (noise (cartesian, true, frequency, lacunarity, persistence, octaves, seed)) * SIMPLEX_SCALE;
}

float billow (vec3 cartesian, float frequency, float lacunarity, float persistence, int octaves, int seed) {
    float value = 0.0;
    float signal = 0.0;
    float curPersistence = 1.0;
    vec3 n = cartesian; // vec3 (makeInt32Range (cartesian.x), makeInt32Range (cartesian.y), makeInt32Range (cartesian.z));

    n *= frequency;

    for (int curOctave = 0; curOctave < octaves; curOctave++) {
        seed = (seed + curOctave) & 0xffffffff;
        signal = cnoise (vec4 (n.xyz, seed));
        signal = 2.0 * abs (signal) - 1.0;
        value += signal * curPersistence;

        // Prepare the next octave.
        n *= lacunarity;
        curPersistence *= persistence;
    }
    return (value + 0.5 + BILLOW_BIAS) * BILLOW_SCALE;
}

vec3 turbulence (vec3 cartesian, float frequency,  float power, int roughness, int seed) {
    // Get the values from the three Perlin noise modules and
    // add each value to each coordinate of the input value.  There are also
    // some offsets added to the coordinates of the input values.  This prevents
    // the distortion modules from returning zero if the (x, y, z) coordinates,
    // when multiplied by the frequency, are near an integer boundary.  This is
    // due to a property of gradient coherent noise, which returns zero at
    // integer boundaries.
    mat3 matrix = mat3 (12414.0, 26519.0, 53820.0, 65124.0, 18128.0, 11213.0, 31337.0, 60493.0, 44845.0);
    matrix /= 65536.0;
    vec3 pos = matrix * cartesian;
    return vec3 (
    cartesian.x + noise (pos, false, frequency, 2.0, 0.5, roughness, seed) * power,
    cartesian.y + noise (pos, false, frequency, 2.0, 0.5, roughness, seed + 1) * power,
    cartesian.z + noise (pos, false, frequency, 2.0, 0.5, roughness, seed + 2) * power);
}

// default values from libnoise: exponent = 1.0, offset = 1.0, gain = 2.0, sharpness = 2.0
float ridgedmulti (vec3 cartesian, float frequency, float lacunarity, int octaves, int seed,
float exponent, float offset, float gain, float sharpness) {

    float pSpectralWeights [30];
    float f = 1.0;
    for (int i = 0; i < 30; i++) {
        // Compute weight for each frequency.
        pSpectralWeights [i] = pow (f, - exponent);
        f *= lacunarity;
    }

    cartesian *= frequency;

    float signal = 0.0;
    float value  = 0.0;
    float weight = 1.0;

    for (int curOctave = 0; curOctave < octaves; curOctave++) {

        // Make sure that these floating-point values have the same range as a 32-
        // bit integer so that we can pass them to the coherent-noise functions.
        vec3 n = cartesian;

        // Get the coherent-noise value.
        int seed = (seed + curOctave) & 0x7fffffff;
        signal = cnoise (vec4 (n.xyz, seed));

        // Make the ridges.
        signal = abs (signal);
        signal = offset - signal;

        // Square the signal to increase the sharpness of the ridges.
        signal = pow (signal, sharpness);

        // The weighting from the previous octave is applied to the signal.
        // Larger values have higher weights, producing sharp points along the
        // ridges.
        signal *= weight;

        // Weight successive contributions by the previous signal.
        weight = clamp (0.0, 1.0, signal * gain);

        // Add the signal to the output value.
        value += (signal * pSpectralWeights [curOctave]);

        // Go to the next octave.
        cartesian *= lacunarity;
    }

    return (((value) - 1.0 + RIDGED_MULTI_BIAS) * RIDGED_MULTI_SCALE) - 1.0;

}

float cylinders (vec3 cartesian, float frequency) {
    cartesian.x *= frequency;
    cartesian.z *= frequency;
    float distFromCenter = sqrt (cartesian.x * cartesian.x + cartesian.z * cartesian.z);
    float distFromSmallerSphere = distFromCenter - floor (distFromCenter);
    float distFromLargerSphere = 1.0 - distFromSmallerSphere;
    float nearestDist = min (distFromSmallerSphere, distFromLargerSphere);
    return 1.0 - (nearestDist * 4.0);
}

float spheres (vec3 cartesian, float frequency) {
    cartesian.x *= frequency;
    cartesian.y *= frequency;
    cartesian.z *= frequency;

    float distFromCenter = sqrt (cartesian.x * cartesian.x + cartesian.y * cartesian.y + cartesian.z * cartesian.z);
    float distFromSmallerSphere = distFromCenter - floor (distFromCenter);
    float distFromLargerSphere = 1.0 - distFromSmallerSphere;
    float nearestDist = min (distFromSmallerSphere, distFromLargerSphere);
    return 1.0 - (nearestDist * 4.0); // Puts it in the -1.0 to +1.0 range.
}

float select (float control, float in0, float in1, float lowerBound, float upperBound, float edgeFalloff) {
    float alpha = smoothstep (-1.0 + edgeFalloff, 1.0 - edgeFalloff, control);
    return mix (in0, in1, alpha);
}

// raster constrained to bounds (a.x, a.y) - (b.x, b.y)
float raster (vec3 cartesian, uint rasterIndex, ivec2 rasterSize) { //, float defaultValue) {
    vec2 rg = toGeolocation (cartesian);

    float dlon = (rg.x + M_PI) / (M_PI * 2);
    float dlat = (rg.y + (M_PI / 2)) / M_PI;
    uvec2 i = uvec2 (uint (dlon * rasterSize.y), uint (dlat * rasterSize.x));
    uint index = i.y * rasterSize.y + i.x;    // rasterSize is QImage::size() - so x is height and y is width
    return rasters [index + rasterIndex];
    //return mix (foundValue, defaultValue, 1.0 - texel.w);  // blend with the default value according to the transparency channel
}

float grid (uint gridIndex, uint gridSize) {
    ivec3 pos = ivec3 (gl_GlobalInvocationID.x, gl_GlobalInvocationID.y, gl_WorkGroupID.z);
    uint i = pos.z * gridSize.x * gridSize.x + pos.y * gridSize.x + pos.x;
    return rasters [i + gridIndex];
}

// Find the x and y coordinates for a geolocation in a raster of height and width given by size and
// spanning the given bounds. If the geolocation is outwith the bounds the returned index will be outwith
// the raster size, so check for this before trying to use the index.
ivec2 index (vec2 g, vec4 bounds, vec2 size) {
    vec2 a = bounds.xy;
    vec2 b = bounds.zw;
    if (a.x > b.x) { // if this is TRUE the raster bounds straddle the dateline
        if (g.x < a.x) {
            g.x += M_PI * 2;
        }
        b.x += M_PI * 2;
    }

    float rx = (g.x - a.x) / (b.x - a.x);
    float ry = (g.y - a.y) / (b.y - a.y);
    return ivec2 (rx * size.x, ry * size.y);
}

// Coordinate systems:
//      screen coordinates (corresponds to GlobalInvoicationID) - the coordinates of the texel being rendered. 0 <= x <= resolution * 2; 0 <= x <= resolution.
//      map coordinates - the coordinates of a point on the logical map. -2 * M_PI < x <= 2 * M_PI; -M_PI <= y <= M_PI.
//          (the logical map is the equirectangular projection with scale 1 unit = 1 radian of latitude or longitude).
//      geolocation - the latitude and longitude (in that order) of a point on the geoid under the reigning projection. -2 * M_PI < x <= 2 * M_PI; -M_PI <= y <= M_PI.
//      cartesian - the point in 3D space corresponding to a geolocation on the surface of the (spherical) geoid. -1 <= x, y, z <= 1 and x^2 + y^2 + z^2 = 1.

// Returns the map coordinates for a given GlobalInvocationID (essentially screen coordinates) taking into account the scale.
// to do - deal with bounds crossing dateline; set bounds via uniform variable; map bounds to globe in the rendering shader

vec2 mapPos (ivec2 pos) {
    vec2 a = bounds.xy;
    vec2 b = bounds.zw;
    if (a.x > b.x) { // if this is TRUE the raster bounds straddle the dateline
        b.x += M_PI * 2;
    }

    float ix = pos.x / size;
    float iy = pos.y / size;
    vec2 g = vec2 ((b.x - a.x) * ix, (b.y - a.y) * iy);
    g += a;
    if (g.x > M_PI) { g.x -= M_PI* 2; }
    return g;
}

// Returns the screen coordinates (GlobalInvocationID) for given map coordinates taking into account the scale.

ivec2 scrPos (vec2 g) {
    vec2 j = g;
    j /= M_PI;
    j.x = (j.x + 1) * size;;
    j.y = (j.y + 0.5) * size;;
    return ivec2 (j);
}


vec4 toGreyscale (vec4 color) {
    float l = sqrt (color.x * color.x + color.y * color.y + color.z * color.z);
    return vec4 (l, l, l, 1.0);
}

// convert a cubemap index to a cartesian vector
// the cubemap index consists of x and y = cell's 2D coordinates on its face and z = the index of the face on which the cell sits
vec3 indexToCartesian (ivec3 fuv) {
    vec2 uv = vec2 ((float (fuv.x) / float (size)) * 2.0f - 1.0f,
        (float (fuv.y) / float (size)  * 2.0f - 1.0f));

    float x, y, z;      // cartesian coordinates on the cube
    if (fuv.z == FACE_NORTH)  { y =  1.0; x = uv.x; z = uv.y; }
    if (fuv.z == FACE_SOUTH)  { y = -1.0; x = uv.x; z = uv.y; }
    if (fuv.z == FACE_EAST)   { x =  1.0; y = uv.x; z = uv.y; }
    if (fuv.z == FACE_WEST)   { x = -1.0; y = uv.x; z = uv.y; }
    if (fuv.z == FACE_FRONT)  { z =  1.0; x = uv.x; y = uv.y; }
    if (fuv.z == FACE_BACK)   { z = -1.0; x = uv.x; y = uv.y; }
    float dx = x * sqrt (1.0f - y * y * 0.5f - z * z * 0.5f + (y * y * z * z) / 3.0f);
    float dy = y * sqrt (1.0f - z * z * 0.5f - x * x * 0.5f + (z * z * x * x) / 3.0f);
    float dz = z * sqrt (1.0f - x * x * 0.5f - y * y * 0.5f + (x * x * y * y) / 3.0f);
    return vec3 (dx, dy, dz);
}


ivec3 cartesianToIndex (vec3 cartesian) {
    vec3 position = cartesian;
    ivec3 fuv;
    float x = position.x, y = position.y, z = position.z;
    float fx = abs (x), fy = abs (y), fz = abs (z);

    if (fy >= fx && fy >= fz) {
        float a2 = x * x * 2.0;
        float b2 = z * z * 2.0;
        float inner = -a2 + b2 - 3;
        float innersqrt = -sqrt ((inner * inner) - 12.0 * a2);
        position.x = (x == 0.0 || x == -0.0) ? 0.0 : (sqrt (innersqrt + a2 - b2 + 3.0) * HALF_ROOT_2);
        position.z = (z == 0.0 || z == -0.0) ? 0.0 : (sqrt (innersqrt - a2 + b2 + 3.0) * HALF_ROOT_2);
        if (position.x > 1.0) { position.x = 1.0; }
        if (position.z > 1.0) { position.z = 1.0; }
        if (x < 0) { position.x = -position.x; }
        if (z < 0) { position.z = -position.z; }
        fuv.z = (y > 0) ? FACE_NORTH : FACE_SOUTH;;
        fuv.x = int ((position.x * 0.5 + 0.5) * size);
        fuv.y = int ((position.z * 0.5 + 0.5) * size);
    } else if (fx >= fy && fx >= fz) {
        float a2 = y * y * 2.0;
        float b2 = z * z * 2.0;
        float inner = -a2 + b2 - 3;
        float innersqrt = -sqrt ((inner * inner) - 12.0 * a2);
        position.y = (y == 0.0 || y == -0.0) ? 0.0 : (sqrt (innersqrt + a2 - b2 + 3.0) * HALF_ROOT_2);
        position.z = (z == 0.0 || z == -0.0) ? 0.0 : (sqrt (innersqrt - a2 + b2 + 3.0) * HALF_ROOT_2);
        if (position.y > 1.0) { position.y = 1.0; }
        if (position.z > 1.0) { position.z = 1.0; }
        if (y < 0) { position.y = -position.y; }
        if (z < 0) { position.z = -position.z; }
        fuv.z = (x > 0) ? FACE_EAST : FACE_WEST;
        fuv.x = int ((position.y * 0.5 + 0.5) * size);
        fuv.y = int ((position.z * 0.5 + 0.5) * size);
    } else {
        float a2 = x * x * 2.0;
        float b2 = y * y * 2.0;
        float inner = -a2 + b2 - 3;
        float innersqrt = -sqrt ((inner * inner) - 12.0 * a2);
        position.x = (x == 0.0 || x == -0.0) ? 0.0 : (sqrt (innersqrt + a2 - b2 + 3.0) * HALF_ROOT_2);
        position.y = (y == 0.0 || y == -0.0) ? 0.0 : (sqrt (innersqrt - a2 + b2 + 3.0) * HALF_ROOT_2);
        if (position.x > 1.0) { position.x = 1.0; }
        if (position.y > 1.0) { position.y = 1.0; }
        if (x < 0) { position.x = -position.x; }
        if (y < 0) { position.y = -position.y; }
        fuv.z = (z > 0) ? FACE_FRONT :  FACE_BACK;
        fuv.x = int ((position.x * 0.5 + 0.5) * size);
        fuv.y = int ((position.y * 0.5 + 0.5) * size);

    }
    return fuv;
}



// inserted code //

void main() {
    ivec3 pos = ivec3 (gl_GlobalInvocationID.x, gl_GlobalInvocationID.y, gl_WorkGroupID.z);
    vec3 c = indexToCartesian (pos);
    vec2 g = toGeolocation (c);
    uint i = pos.z * size * size + pos.y * size + pos.x;
    height_map_out [i] = value (pos, c, g);
}
