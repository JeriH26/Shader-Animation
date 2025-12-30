float sdRoundedBox( in vec2 p, in vec2 b, in vec4 r )
{
    r.xy = (p.x>0.0)?r.xy : r.zw;
    r.x  = (p.y>0.0)?r.x  : r.y;
    vec2 q = abs(p)-b+r.x;
    return min(max(q.x,q.y),0.0) + length(max(q,0.0)) - r.x;
}

float sdCircle( vec2 p, float r )
{
    return length(p) - r;
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord / iResolution.xy-.5;
    float ratio = iResolution.x / iResolution.y;
    uv = vec2(uv.x * ratio, uv.y);

    vec2 si = vec2(0.05,0.0) + 0.3*cos(iTime*vec2(1,1));
    vec4 ra = 0.2 + 0.2*cos(2.0+ vec4(0,1,2,3) );
    ra = min(ra,min(si.x,si.y));

    float d = sdRoundedBox(uv, si, ra);
    vec3 col = (d>0.0) ? vec3(0.9,0.6,0.3) : vec3(0.65,0.85,1.0);
    //col *= 1.-exp(-6.0*abs(d));
	//col *= 0.8 + 0.2*cos(150.0*d);
	//col = mix( col, vec3(1.0), 1.0-smoothstep(0.0,0.01,abs(d)) );

    //vec3 col = vec3(pct);

    // Time varying pixel color
    //col *= 0.5 + 0.5 * cos(iTime + uv.xyx + vec3(0, 2.0, 4.0));

    // Output to screen
    fragColor = vec4(col, 1.0);
}