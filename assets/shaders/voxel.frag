#version 330

in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragLocal;
in vec3 fragLocalNormal;
in vec3 fragJitter;

out vec4 finalColor;

uniform sampler2D texture0;
uniform sampler2D texture1;
uniform vec4 colDiffuse;
uniform float lightAmbient;

uniform float spriteLit;
uniform vec3 spriteAt;
uniform vec3 spriteFrom;
uniform vec3 spriteSpan;

const int mostLamps = 8;

// The shadow-atlas shape arrives as uniforms so the layout math
// tracks light::kMaxLamps and gfx::kCubeFaces; the array size
// above must stay a compile-time constant.
uniform float lampSlots;
uniform float lampFaces;

uniform vec3 lampAt[mostLamps];
uniform vec4 lampTint[mostLamps];
uniform float lampsLit;
uniform float lampReach[mostLamps];
uniform float lampBrightness[mostLamps];
uniform float lampStrength;

uniform float lampShadows[mostLamps];

uniform vec3 sightPoint;
uniform float sightSlot;

uniform vec3 upperSightPoint;
uniform float upperSightSlot;
uniform float upperSightOn;

uniform float sightOn;

uniform float walkerAt;
uniform float walkerLight;
uniform float walkerLightRange;
uniform float levelFade;

uniform float nightEdge;
uniform vec3 fadeFrom;
uniform float fadeReach;

uniform float carrying;

uniform float glowOnly;

uniform vec3 fogFrom;
uniform vec3 fogWay;
uniform vec4 fogTint;
uniform float fogNear;
uniform float fogFar;
uniform float fogStrength;

// Depth is measured along the way the camera looks, from the plane
// through what it aims at, so the fog reads as distance behind the
// walker rather than as distance from the middle of the screen.
float foggedAt(vec3 drawnAt)
{
    float behind = dot(drawnAt - fogFrom, fogWay);

    return fogStrength * smoothstep(fogNear, fogFar, behind);
}

uniform sampler2D texture2;
uniform vec3 hidingCorner;
uniform float hidingSpan;
uniform float hidingLevels;

bool hiddenAt(vec3 stood, vec3 outward)
{
    vec3 cell = floor(stood - (outward * 0.25));
    vec2 onMask = cell.xz - hidingCorner.xz;

    if (any(lessThan(onMask, vec2(0.0)))
        || any(greaterThanEqual(onMask, vec2(hidingSpan)))
        || cell.y < 0.0 || cell.y >= hidingLevels)
    {
        return false;
    }

    vec4 written = texture(
        texture2, (onMask + vec2(0.5)) / hidingSpan);
    int level = int(cell.y);

    // Round before extracting: the UNORM8 read can land just
    // below the stored byte and floor would then drop a bit.
    float byte = floor((written[level / 8] * 255.0) + 0.5);

    float bit = float(level - ((level / 8) * 8));

    return mod(floor(byte / pow(2.0, bit)), 2.0) >= 0.5;
}

uniform sampler2D texture3;
uniform float lampFaceSide;
uniform float lampBias;
uniform mat4 lampViewEast;
uniform mat4 lampViewWest;
uniform mat4 lampViewUp;
uniform mat4 lampViewDown;
uniform mat4 lampViewSouth;
uniform mat4 lampViewNorth;

mat4 lampViewOf(int way)
{
    if (way == 0) { return lampViewEast; }
    if (way == 1) { return lampViewWest; }
    if (way == 2) { return lampViewUp; }
    if (way == 3) { return lampViewDown; }
    if (way == 4) { return lampViewSouth; }

    return lampViewNorth;
}

int wayOf(vec3 held)
{
    vec3 far = abs(held);

    if (far.x >= far.y && far.x >= far.z)
    {
        return held.x > 0.0 ? 0 : 1;
    }

    if (far.y >= far.z)
    {
        return held.y > 0.0 ? 2 : 3;
    }

    return held.z > 0.0 ? 4 : 5;
}

vec3 nudgedAway(vec3 held, vec3 outward, float turned)
{
    float span = max(length(held), 0.0001);
    float texel = 2.0 * span / lampFaceSide;
    float laidOver = min(
        sqrt(max(1.0 - turned * turned, 0.0)) / max(turned, 0.05),
        8.0);

    return held + outward * texel * (1.0 + laidOver);
}

float lampWrote(vec2 faceTexel, vec2 corner, float mine)
{
    vec2 sheet = lampFaceSide * vec2(lampFaces, lampSlots);

    return mine - lampBias
                   > texture(texture3, (corner + faceTexel) / sheet)
                         .r
               ? 0.0
               : 1.0;
}

float lampReaching(int lamp, vec3 held)
{
    int way = wayOf(held);
    vec4 clip = lampViewOf(way) * vec4(held, 1.0);

    if (clip.w <= 0.0)
    {
        return 1.0;
    }

    vec3 landed = clip.xyz / clip.w;

    if (any(greaterThan(abs(landed), vec3(1.0))))
    {
        return 1.0;
    }

    vec2 onFace = landed.xy * 0.5 + 0.5;

    vec2 edge = vec2(2.5) / vec2(lampFaceSide);
    onFace = clamp(onFace, edge, vec2(1.0) - edge);

    vec2 corner =
        vec2(float(way), lampSlots - float(lamp) - 1.0) * lampFaceSide;

    float mine = landed.z * 0.5 + 0.5;
    vec2 texel = onFace * lampFaceSide;
    float reached = 0.0;

    for (int down = 0; down < 2; ++down)
    {
        for (int across = 0; across < 2; ++across)
        {
            vec2 spread =
                texel
                + ((vec2(float(across), float(down)) - vec2(0.5))
                   * 1.5);
            vec2 base = floor(spread - vec2(0.5)) + vec2(0.5);
            vec2 part = spread - base;

            float nw = lampWrote(base, corner, mine);
            float ne =
                lampWrote(base + vec2(1.0, 0.0), corner, mine);
            float sw =
                lampWrote(base + vec2(0.0, 1.0), corner, mine);
            float se =
                lampWrote(base + vec2(1.0, 1.0), corner, mine);

            reached += mix(
                mix(nw, ne, part.x), mix(sw, se, part.x), part.y);
        }
    }

    return reached / 4.0;
}

// The corners wobble in the vertex stage but the painting must not:
// the texels stay pinned to the flat plane the face had before its
// corners moved. The wobbled offset is carried back onto that plane
// along the way the camera looks, and the uv is re-read where the
// ray lands, so the texel grid holds still and only the outline
// bends. fragTexCoord and fragLocal share one interpolation, so the
// screen derivatives give the exact uv-per-plane-step ratio.
vec2 anchoredUv()
{
    vec3 alongX = dFdx(fragLocal);
    vec3 alongY = dFdy(fragLocal);
    vec2 paintX = dFdx(fragTexCoord);
    vec2 paintY = dFdy(fragTexCoord);

    vec3 outward = normalize(fragLocalNormal);
    float toward = dot(fogWay, outward);

    float xx = dot(alongX, alongX);
    float xy = dot(alongX, alongY);
    float yy = dot(alongY, alongY);
    float spread = (xx * yy) - (xy * xy);

    if (abs(toward) < 0.05 || spread < 1e-12)
    {
        return fragTexCoord;
    }

    vec3 slide =
        fragJitter - (fogWay * (dot(fragJitter, outward) / toward));

    float onX = ((dot(slide, alongX) * yy) - (dot(slide, alongY) * xy))
                / spread;
    float onY = ((dot(slide, alongY) * xx) - (dot(slide, alongX) * xy))
                / spread;

    return fragTexCoord + (onX * paintX) + (onY * paintY);
}

// The atlases keep two blank pixels between tiles, so a sample that
// slid past its tile edge is pulled back inside. The shape mirrors
// tilemap::AtlasLayout: sixteen columns and rows with the padding
// only between tiles, so the tile size falls out of the sheet size.
const float kSheetTiles = 16.0;
const float kSheetPadding = 2.0;

vec2 clampedToTile(vec2 uv, vec2 sheetSize)
{
    vec2 tileSize =
        (sheetSize - ((kSheetTiles - 1.0) * kSheetPadding))
        / kSheetTiles;
    vec2 stride = tileSize + vec2(kSheetPadding);
    vec2 cornerPixel =
        floor((fragTexCoord * sheetSize) / stride) * stride;

    return clamp(
               uv * sheetSize,
               cornerPixel + vec2(0.5),
               (cornerPixel + tileSize) - vec2(0.5))
           / sheetSize;
}

float sightReaching(int slot, vec3 point, vec3 stood, vec3 outward)
{
    vec3 toSight = point - stood;
    float sightSpan = max(length(toSight), 0.0001);
    float sightTurned = max(dot(outward, toSight / sightSpan), 0.0);

    return lampReaching(
        slot, nudgedAway(stood - point, outward, sightTurned));
}

void main()
{
    vec3 normal = normalize(fragNormal);
    bool sprite = spriteLit > 0.5;

    vec3 stood = sprite ? spriteAt : fragLocal;
    vec3 facing = sprite ? vec3(0.0, 1.0, 0.0) : fragLocalNormal;

    vec3 outward = normalize(facing);

    vec2 anchored = anchoredUv();

    vec4 skin =
        sprite
            ? texture(
                  texture0,
                  spriteFrom.xy
                      + (fragTexCoord * spriteSpan.xy))
        : abs(normal.y) > 0.5
            ? texture(
                  texture0,
                  clampedToTile(
                      anchored, vec2(textureSize(texture0, 0))))
            : texture(
                  texture1,
                  clampedToTile(
                      anchored, vec2(textureSize(texture1, 0))));

    if (skin.a < 0.5)
    {
        discard;
    }

    if (!sprite && hiddenAt(fragLocal, outward))
    {
        discard;
    }

    float glow =
        sprite ? 0.0 : clamp((1.0 - skin.a) * 2.55, 0.0, 1.0);

    vec3 glowing = skin.rgb * fragColor.rgb * glow;

    float left = 1.0;

    if (nightEdge > 0.5)
    {
        float off = distance(stood.xz, fadeFrom.xz);
        left = 1.0
               - smoothstep(fadeReach * 0.55, fadeReach, off);
    }

    if (glowOnly > 0.5 && glow <= 0.0)
    {
        finalColor = vec4(0.0, 0.0, 0.0, 1.0);

        return;
    }

    float seen = 1.0;

    if (sightOn > 0.5 && !sprite)
    {
        seen = sightReaching(int(sightSlot), sightPoint, stood, outward);

        if (upperSightOn > 0.5)
        {
            seen = max(
                seen,
                sightReaching(
                    int(upperSightSlot), upperSightPoint, stood, outward));
        }
    }

    if (glowOnly > 0.5)
    {
        finalColor = vec4(glowing * seen * left, 1.0);

        return;
    }

    vec3 lamplight = vec3(0.0);

    int lit = int(lampsLit);

    for (int at = 0; at < lit; ++at)
    {
        vec3 toLamp = lampAt[at] - stood;
        float span = max(length(toLamp), 0.0001);
        float turned = max(dot(outward, toLamp / span), 0.0);
        float reachLeft =
            clamp(1.0 - (span / lampReach[at]), 0.0, 1.0);

        float spare = smoothstep(0.0, 1.0, sqrt(reachLeft));

        if (turned <= 0.0 || spare <= 0.0 || lampBrightness[at] <= 0.0)
        {
            continue;
        }

        float got =
            lampShadows[at] < 0.5 || (sprite && at == int(carrying))
                ? 1.0
                : lampReaching(
                      at,
                      nudgedAway(stood - lampAt[at], outward, turned));

        lamplight += lampTint[at].rgb * lampBrightness[at] * turned
                     * spare * lampStrength * got;
    }

    float nearLeft = clamp(
        1.0 - (distance(stood, fadeFrom) / walkerLightRange), 0.0, 1.0);
    vec3 nearby =
        vec3(walkerLight * smoothstep(0.0, 1.0, sqrt(nearLeft)));

    float apart = sprite ? 0.0 : abs(stood.y - walkerAt);
    float onLevel = max(1.0 - (apart * levelFade), 0.0);

    vec3 shade = skin.rgb * fragColor.rgb * seen * onLevel
                 * (vec3(lightAmbient) + lamplight + nearby);

    shade += glowing * seen;
    shade *= left;
    shade = mix(shade, fogTint.rgb, foggedAt(fragPosition));

    float veil = sprite ? skin.a : 1.0;

    finalColor = vec4(shade, veil * fragColor.a) * colDiffuse;
}
