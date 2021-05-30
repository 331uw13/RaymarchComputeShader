#version 430 core

uniform float time;

#define FOV 60.0
#define MAX_STEPS    100
#define MAX_RAY_LEN  300.0


#define PI 3.14159
#define PI_R (PI/180.0)



layout (local_size_x = 1, local_size_y = 1) in;
layout (rgba32f, binding = 0) uniform image2D img;


float sphereSDF(vec3 p, float rad) {
	return length(p)-rad;
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

vec3 raydir(vec2 px_coord, vec2 img_size) {
	vec2 rs = img_size*0.5;
	float hf = tan((90.0-FOV*0.5)*PI_R);
	return normalize(vec3(px_coord-rs, rs.y*hf));
}


// https://iquilezles.org/www/articles/fog/fog.html
vec3 add_fog(vec3 color, float point_dist, float strength) {
	vec3 fog_color = vec3(0.5, 0.8, 0.7);
	return mix(color, fog_color, 1.0-exp(-point_dist*strength));
}


vec4 map(vec3 p) {

	p.xz *= rm2(time*0.115);
	p.z += time*18.0;
	vec3 r = rep(p, vec3(6.0));

	float d1 = sphereSDF(r, 1.0);
	vec4 vd1 = vec4(vec3(1.0, 0.5, 0.2), d1);


	return vd1;
}


vec3 compute_normal(vec3 p) {
	vec2 e = vec2(0.01, 0.0);
	vec3 v = vec3(map(p-e.xyy).w, map(p-e.yxy).w, map(p-e.yyx).w);
	return normalize(map(p).w-v);
}


void main() {	
	vec3 px_color = vec3(0.0);
	vec2 px_coord = gl_GlobalInvocationID.xy;
	vec2 img_size = imageSize(img);

	vec3 rd = raydir(px_coord, img_size);
	vec3 ro = vec3(0.0, 0.0, -5.0);
	rd.xy *= rm2(time*0.3);

	float ray_len = 0.0;
	
	for(int i = 0; i < MAX_STEPS; i++) {
		vec3 p = ro+rd*ray_len;
		vec4 closest = map(p);

		if(closest.w <= (0.005*ray_len)) {
			px_color = (1.0-compute_normal(p))*closest.xyz;
			break;
		}

		if(ray_len >= MAX_RAY_LEN) { 
			break;
		}

		ray_len += max(abs(closest.w), 0.01);
	}

	px_color = add_fog(px_color, ray_len, 0.02);


	imageStore(img, ivec2(px_coord), vec4(px_color, 1.0));
}


