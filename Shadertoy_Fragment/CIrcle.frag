
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

  //左右边界
  float x1 = step(left,st.x);
  float x2 = step(right,1.0-st.x); //检测值要小于右边界才应该返回1.0，所以使用1.0-st.x
  
  //上下边界
  float y1 = step(bottom,st.y);
  float y2 = step(top,1.0-st.y);//检测值要小于上边界才应该返回1.0，所以使用1.0-st.y

  float pct = x1 * x2 *y1 *y2;
  return pct;
}

float box2(vec2 st){

  float left = 0.;
  float right = 0.4;
  float top = 0.6;
  float bottom = 0.2;

  //左下边界
  vec2 bl = step(vec2(left,bottom),st);
  float pct = bl.x * bl.y;


  //右上边界
  vec2 tr = step(vec2(right,top),1.0-st);//检测值要小于右上边界才应该返回1.0，所以使用1.0-st
    pct *= tr.x * tr.y;

  return pct;
}

float box3(vec2 st){

   float right = 0.4;
   float top = 0.1;

   //通过右上角绘制原点对称的四边形
    vec2 bl = 1.0-step(vec2(right,top),abs(st));
   //vec2 bl = step(vec2(right,top),abs(st));
   float pct = bl.x * bl.y;

   return pct;
 }

float box4(vec2 st){

  float right = 0.4;
  float top = 0.2;
  float line_width = 0.01;
  
  //通过右上角绘制原点对称的四边形
  vec2 b1 = 1.0-step(vec2(right,top),abs(st));

  float boxouter = b1.x * b1.y;

  vec2 b2 = 1.0-step(vec2(right-line_width,top-line_width),abs(st));
  float boxinner = b2.x * b2.y;

  float pct = boxouter -boxinner;

  return pct;
}


float circle(vec2 st,vec2 center,float radius) { 
  float blur = 0.002;

  //float pct = distance(st,center);//计算任意点到圆心的距离

  vec2 tC = st-center; //计算圆心到任意点的向量
  float pct = length(tC);//使用length函数求出长度
  //float pct = sqrt(tC.x*tC.x+tC.y*tC.y);//使用开平方的方法求出长度

  return 1.-smoothstep(radius-blur,radius+blur,pct);
  //return 1.-pct;
}  

float circleLine(vec2 st,vec2 center,float radius) { 
  float pct = distance(st,center);//计算任意点到圆心的距离
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