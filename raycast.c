#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "structures.h"
#include "ppmrw.c"
#include "parser.c"

/*
  Finds sphere intersection point with given ray
  Ro: Ray origin vector
  Rd: Ray direction vector
  C: Sphere center position vector
  r: Sphere radius
*/
double sphere_intersection(double* Ro, double* Rd, double* C, double r){
	double a = sqr(Rd[0]) + sqr(Rd[1]) + sqr(Rd[2]);
	double b =  2 * (Rd[0] * (Ro[0] - C[0]) + Rd[1] * (Ro[1] - C[1]) + Rd[2] * (Ro[2] - C[2]));
	double c = sqr(Ro[0] - C[0]) + sqr(Ro[1] - C[1]) + sqr(Ro[2] - C[2]) - sqr(r);
	double det = sqr(b) - 4 * a * c;
	if (det < 0) return -1;
  
	det = sqrt(det);

	// Tests which is the closest one and in front of camera.
	double t = (-b - det)/(2 * a);
	if (t > 0) return t;
	t = (-b + det)/(2 * a);
	if (t > 0) return t;

	return -1;
}

/*
  Finds plane intersection point with given ray
  Ro: Ray origin vector
  Rd: Ray direction vector
  p: Plane position vector
  n: Plane normal vector

*/
double plane_intersection(double* Ro, double* Rd, double* p, double* n){
	// a(xr - x0) + b(yr - y0,) + c(zr - z0) = 0
	double top = (n[0] * Ro[0] - n[0] * p[0] + n[1] * Ro[1] - n[1] * p[1] + n[2] * Ro[2] - n[2] * p[2]);
	double bottom = (n[0] * Rd[0] + n[1] * Rd[1] + n[2] * Rd[2]);
	double t = -1 * top / bottom;

	if (t > 0) return t;
  
	return -1;
}
/*
  Casts ray to find first interaction with object
  ray_len: The double address to return the length of the ray to.
  objects: The objects in the scene.
  Ro: The origin vector of the ray.
  Rd: The direction vector of the ray.
 */
Object* cast_ray(double* ray_len, Object** objects, double* Ro, double* Rd){

	double best_t = INFINITY;
	int best_i = -1;

	// Finds the closest object on the ray
	for (int i=0; objects[i] != 0; i += 1) {
		double t = 0;

		switch(objects[i]->id) {
		case 2 :
			;
			Sphere* s = (Sphere*) objects[i];
			t = sphere_intersection(Ro, Rd,
									s->pos,
									s->radius);
			break;
		case 3 :
			;
			Plane* p = (Plane*) objects[i];
			t = plane_intersection(Ro, Rd,
								   p->pos,
								   p->normal);
			break;
		default:
			fprintf(stderr, "Unsupported object during rendering with Id: %d\n", objects[i]->id);
			exit(1);
		}
		if (t > 0 && t < best_t){
			best_t = t;
			best_i = i;
		}
	}

	// Gets the closest object for color. 
	if (best_t > 0 && best_t != INFINITY && best_i != -1) {
		*ray_len = best_t;
		return objects[best_i];
	} else {
		*ret_t = -1;
		return NULL;
	}
}

/*
  Paints the scene to an image using ray casting
  scene: The scene to be painted
  height: the height of the final image
  width: the width of the final image
*/
Image* paint_scene(Scene* scene, int height, int width) {
	Object** objects = scene->objects;
	double cx = 0;
	double cy = 0;
	double h = scene->cam->height;
	double w = scene->cam->width;

	int M = height;
	int N = width;
  
	Image* img = malloc(sizeof(Image));
	img->width = N;
	img->height = M;
	img->max_value = 255;
	img->buffer = malloc(sizeof(Pixel) * M * N);
	
	// Finds the color for each pixel and stores it
	double pixheight = h / M;
	double pixwidth = w / N;
	for (int y = 0; y < M; y += 1) {
		for (int x = 0; x < N; x += 1) {
			double Ro[3] = {0, 0, 0};
			// Rd = normalize(P - Ro)
			double Rd[3] = {
				cx + (w/2) - pixwidth * (x + 0.5),
				cy + (h/2) - pixheight * (y + 0.5),
				1
			};
			normalize(Rd);

			double t;
			Object* object = cast_ray(&t, objects, Ro, Rd);
			Pixel pix;
			if (object != NULL){
				double* color = ((Sphere*)object)->color;
				pix.r = color[0];
				pix.g = color[1];
				pix.b = color[2];
			} else {
				pix.r = 0;
				pix.g = 0;
				pix.b = 0;
			}
			img->buffer[y * N + x] = pix;
		}
	}
	return img;
}


int main(int argc, char* argv[]){

	if (argc != 5){
		fprintf(stderr, "Proper Usage: raycast width height input.json output.ppm\n");
		exit(1);
	}

	// Checks if supplied arguments are valid
	int width, height;
	if (width = atoi(argv[1]), width <= 0){
		fprintf(stderr, "Error: Invalid width.\n");
		exit(1);
	}
	
	if (height = atoi(argv[2]), height <= 0){
		fprintf(stderr, "Error: Invalid height.\n");
		exit(1);
	}

	FILE* out = fopen(argv[4],"wb");
	if (out == NULL){
		fprintf(stderr, "Error: Output file write access.\n");
		exit(1);
	}

	// Reads in scene file
	Scene* scene = read_scene(argv[3]);

	// Checks if scene has camera
	if (scene->cam == NULL){
		fprintf(stderr,"Error: No camera found.\n");
		exit(1);
	}

	Plane* plane = ((Plane*) scene->objects[4]);
	double* color = plane->color;
	// Paints scene into image file using raycasting
	Image* img = paint_scene(scene, height, width);

	// Write image to file
	write_file(out, img, 6);
}