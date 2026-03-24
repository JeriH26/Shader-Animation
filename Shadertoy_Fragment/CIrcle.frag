
float box0(vec2 st){
  float left = 0.0;
  float right = 0.4;
  float top = 0.6;
  float bottom = 0.2;
  if(st.x > left && st.x < right && st.y > bottom && st.y < top){
    return 1.0;
  }else{
    return 0.0;
  } 
}


float box1(vec2 st){

  float left = 0.0;
  float right = 0.4;
  float top = 0.6;
  float bottom = 0.2;

  // Left/right boundaries
  float x1 = step(left,st.x);
  float x2 = step(right,1.0-st.x); // The tested value must be below the right boundary, so we use 1.0 - st.x
  
  // Bottom/top boundaries
  float y1 = step(bottom,st.y);
  float y2 = step(top,1.0-st.y); // The tested value must be below the top boundary, so we use 1.0 - st.y

  float pct = x1 * x2 *y1 *y2;
  return pct;
}

float box2(vec2 st){

  float left = 0.;
  float right = 0.4;
  float top = 0.6;
  float bottom = 0.2;

  // Lower-left boundary
  vec2 bl = step(vec2(left,bottom),st);
  float pct = bl.x * bl.y;


  // Upper-right boundary
  vec2 tr = step(vec2(right,top),1.0-st); // The tested value must be below the upper-right boundary, so use 1.0 - st
    pct *= tr.x * tr.y;

  return pct;
}

float box3(vec2 st){

   float right = 0.4;
   float top = 0.1;

   // Draw a quad symmetric around the origin using the upper-right corner
    vec2 bl = 1.0-step(vec2(right,top),abs(st));
   //vec2 bl = step(vec2(right,top),abs(st));
   float pct = bl.x * bl.y;

   return pct;
 }

float box4(vec2 st){

  float right = 0.4;
  float top = 0.2;
  float line_width = 0.01;
  
  // Draw a quad symmetric around the origin using the upper-right corner
  vec2 b1 = 1.0-step(vec2(right,top),abs(st));

  float boxouter = b1.x * b1.y;

  vec2 b2 = 1.0-step(vec2(right-line_width,top-line_width),abs(st));
  float boxinner = b2.x * b2.y;

  float pct = boxouter -boxinner;

  return pct;
}


float circle(vec2 st,vec2 center,float radius) { 
  float blur = 0.002;

  //float pct = distance(st,center); // Distance from any point to the circle center

  vec2 tC = st-center; // Vector from center to any point
  float pct = length(tC); // Compute the length with length()
  //float pct = sqrt(tC.x*tC.x+tC.y*tC.y); // Equivalent length via square root

  return 1.-smoothstep(radius-blur,radius+blur,pct);
  //return 1.-pct;
}  

float circleLine(vec2 st,vec2 center,float radius) { 
  float pct = distance(st,center); // Distance from any point to the circle center
  float line_width = 0.01;
  float radius2 = radius-line_width;
  float blur = 0.002;
  return (1.0-smoothstep(radius-blur,radius+blur,pct))-(1.0-smoothstep(radius2-blur,radius2+blur,pct));
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord / iResolution.xy-.5;
    float ratio = iResolution.x / iResolution.y;

    float pct = circleLine(vec2(uv.x * ratio, uv.y), vec2(0,0),.1);
    vec3 col = vec3(pct);

    // Time varying pixel color
    col *= 0.5 + 0.5 * cos(iTime + uv.xyx + vec3(0, 2.0, 4.0));

    // Output to screen
    fragColor = vec4(col, 1.0);
}