//#version 300 es
// using this because it is required if we want to use "binding" qualifier in layout (can be set in cpp code but it is easier to do in shader, so procedd like this and maybe change later)
//#version 420

#define PREC highp

#include <include/lighting.hglsl>

// x: the shape's alpha (it fades); y: 1 for a HUD element (the compass), at full
// light and out of the fog, as the CPU path draws it
uniform PREC vec4 mesh_params_;

in PREC vec3 Normal;
//in PREC float FogValue;
in PREC vec2 Texcoord;
in PREC vec4 VertexColor;
in PREC vec3 VertexLight;
in PREC vec3 WorldPos;
in PREC vec3 CameraPos;

layout (location=0) out PREC vec4 FragColor;

uniform sampler2D tex1;
uniform PREC vec4 fog_color;

in PREC float ModelHeight;

// VR: skybox is an open ring from -261 to 510 high, round the camera, so 0 is
// the horizon. It fades to the colour each eye clears to at its top, which a
// headset can see, and at the horizon, where the ground has faded to that too.
const PREC float SKY_TOP_FADE_START = 300.0;
const PREC float SKY_TOP = 510.0;
const PREC float SKY_HORIZON = 0.0;
const PREC float SKY_HORIZON_FADE_END = 120.0;

void main(void)
{
    PREC vec4 c = vec4(1,1,1,1);//Color.bgra;
    PREC vec4 tex_color = texture(tex1, Texcoord);
    c *= tex_color;
    
#ifdef ALPHA_TEST
    if(tex_color.a == 0.0)
        discard;
#endif

	//if(fog_color.x>0.0 || fog_color.y>0.0 || fog_color.z>0.0 || fog_color.w>0.0)
    //	c.rgb = mix(fog_color.rgb, c.rgb, FogValue);

#if ENABLE_VERTEX_LIGHTING
	PREC vec3 lighting = VertexLight;
#else
    PREC vec3 lighting = calc_light(Normal, VertexLight);
#endif

    if(mesh_params_.y == 0.0)
    {
        c.xyz = c.xyz * lighting;
        c.xyz = apply_fog(c.xyz, WorldPos.xyz, CameraPos);
        if(mesh_params_.z > 0.0)
        {
            PREC float sky_fade = max(smoothstep(SKY_TOP_FADE_START, SKY_TOP, ModelHeight),
                    1.0 - smoothstep(SKY_HORIZON, SKY_HORIZON_FADE_END, ModelHeight));
            c.xyz = mix(c.xyz, g_scene.fogColor.rgb, sky_fade);
        }
    }

	FragColor = vec4(c.xyz, c.a * mesh_params_.x);
}

