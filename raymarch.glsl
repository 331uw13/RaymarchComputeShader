#version 430 core

uniform float time;
uniform vec2 camera;
uniform vec3 player;

#define FOV 60.0
#define MAX_RAY_LEN  300.0


#define PI 3.14159
#define PI_R (PI/180.0)



layout (local_size_x = 1, local_size_y = 1) in;
layout (rgba32f, binding = 0) uniform image2D img;


float sphereSDF(vec3 p, float rad) {
	return length(p)-rad;
}

float boxSDF(vec3 p, vec3 size) {
	vec3 q = abs(p)-size;
	return length(max(q, 0.0))+min(max(q.x, max(q.y, q.z)), 0.0);
}

vec4 minSDF(vec4 a, vec4 b) {
	return (a.w <= b.w) ? a : b;
}

vec3 rep(vec3 p, vec3 f) {
	return mod(p, f)-0.5*f;
}


mat2 rm2(float a) {
	float c = cos(a);
	float s = sin(a);
	return mat2(c, -s, s, c);
}

mat3 rm3(vec2 a) {
	vec2 c = cos(a);
	vec2 s = sin(a);
	return mat3(c.y, 0.0, -s.y, s.y*s.x, c.x, c.y*s.x, s.y*c.x, -s.x, c.y*c.x);
}

mat4 view_mat(vec3 eye, vec3 center, vec3 up) {
	vec3 f = normalize(center-eye);
	vec3 s = normalize(cross(f, up));
	vec3 u = cross(s, f);
	return mat4(vec4(s, 0.0), vec4(u, 0.0), vec4(-f, 0.0), vec4(0.0, 0.0, 0.0, 1.0));
}

vec3 raydir(vec2 px_coord, vec2 img_size) {
	vec2 rs = img_size*0.25;
	float hf = tan((90.0-FOV*0.5)*PI_R);
	return normalize(vec3(px_coord-rs, rs.y*hf));
}


// https://iquilezles.org/www/articles/fog/fog.html
vec3 add_fog(vec3 color, float point_dist, float strength) {
	vec3 fog_color = vec3(0.3, 0.4, 0.3);
	return mix(color, fog_color, pow(1.0-exp(-point_dist*strength), 3.0));
}


float rand(vec2 co){
	return fract(sin(dot(co.xy, vec2(12.98,28.233)))*458.5453);
}

vec4 map(vec3 p) {

	p.y += 15.0;

	vec3 tmp = rep(p, vec3(1.0, 30.0, 1.0));
	p.y = tmp.y;


	vec3 r   = rep(p, vec3(40.0));
	vec3 r2  = rep(p+vec3(10.0,0.0,0.0), vec3(20.0, 1.0, 5.0));
	vec3 r3  = rep(p+vec3(0.0,0.0,10.0), vec3(5.0, 1.0, 20.0));
	
	
	vec3 r4  = rep(p+vec3(0.0,0.0,time*80.0), vec3(1.0, 1.0, 5.0));

	vec3 qq = p;
	qq.xy *= rm2(5.0*time+p.z*3.0*PI_R);

	vec3 q = abs(qq)-vec3(2.0,2.0,0.0);


	r.y = p.y;
	r2.y = p.y;
	r2.z = p.z;
	r3.y = p.y;
	r3.x = p.x;

	float d1 = boxSDF(r, vec3(1.3,10.0,1.3))-0.2;
	float d2 = boxSDF(abs(r)-vec3(0.0,10.0,0.0), vec3(10.0,0.1,10.0));
	
	
	d2 = min(d2, boxSDF(abs(r)-vec3(0.0,10.5,0.0), vec3(5.0,1.5,5.0))-0.15);
	
	float d3 = boxSDF(abs(r2)-vec3(0.0,10.0,0.0), vec3(1.0, 1.0, abs(p.z)));
	float d4 = min(d3, boxSDF(abs(r3)-vec3(0.0,10.0,0.0), vec3(abs(p.x), 0.4, 0.4)));

	//r.xz *= rm2(time-p.y*0.3);
	float d5 = boxSDF((abs(r)-vec3(1.75,0.0,1.75)), vec3(0.3,10.0,0.3))-0.1;
	
	float d6 = boxSDF(q, vec3(0.001, 0.001, abs(p.z)))-0.3;
	float d7 = sphereSDF(vec3(p.x, p.y, r4.z), 2.5);


	vec3 rainbow = 0.5+0.5*cos(time*0.3+vec3(0,2,4));

	vec3 power_color = vec3(1.0,0.1,0.0)-(0.3+0.3*floor(cos(time*50.0+p.z*4.0)));
	vec3 pilar_color = vec3(0.3, 1.5, 0.1);
	vec3 floor_color = vec3(0.3);


	float t = time*5.0;
	float anim = abs(sin(p.y*0.5+time)-cos(p.x*0.1+time*0.3+p.z*0.1+time*0.5));
	
	anim = smoothstep(0.03, 1.0, anim);
	pilar_color.xy -= 0.05*(anim*10.0);


	vec4 vd1 = vec4(pilar_color, d1);
	vec4 vd2 = vec4(floor_color, d2);
	vec4 vd3 = vec4(power_color, d3);
	vec4 vd4 = vec4(power_color*0.5, d4);
	vec4 vd5 = vec4(vec3(0.2), d5);
	vec4 vd6 = vec4(vec3(1.0), d6);
	vec4 vd7 = vec4(0.5+0.5*cos(length(p.z+time*3.0)*0.1+time*0.2+vec3(0,2,4)), d7);

	return minSDF(vd1, minSDF(vd3, minSDF(vd4, minSDF(vd5, minSDF(vd6, vd7)))));
}


vec3 compute_normal(vec3 p) {
	vec2 e = vec2(0.001, 0.0);
	vec3 v = vec3(map(p-e.xyy).w, map(p-e.yxy).w, map(p-e.yyx).w);
	return normalize(map(p).w-v);
}

vec3 compute_light(vec3 p, vec3 ro, vec3 pos, vec3 color) {
	vec3 ambient = color*0.3;
	vec3 diffuse = color;
	
	vec3 ldir = normalize(pos-p);
	vec3 n = compute_normal(p);
	return (ambient+diffuse*max(0.0,dot(ldir, n)));
}

vec3 add_lights(vec3 p, vec3 ro) {
	
	vec3 res = vec3(0.0);
	res += compute_light(p, ro, ro, vec3(1.0, 1.0, 1.0));

	return res;
}



void main() {
	vec3 px_color = vec3(0.0);
	vec2 px_coord = gl_GlobalInvocationID.xy;
	vec2 img_size = imageSize(img);

	vec3 rd = raydir(px_coord, img_size);
	vec3 ro = vec3(player.x, 0.0, player.z);

	rd.zy *= rm2(camera.y);
	rd.xz *= rm2(camera.x);

	float rl = 0.0;
	for(; rl < MAX_RAY_LEN; ) {
		vec3 p = ro+rd*rl;
		vec4 closest = map(p);

		if(closest.w <= 0.001) {
			px_color = add_lights(p, ro)*closest.xyz;
			break;
		}
		
		rl += closest.w;
	}
	
	px_color = add_fog(px_color, rl, 0.03);
	imageStore(img, ivec2(px_coord), vec4(px_color, 1.0));
}


