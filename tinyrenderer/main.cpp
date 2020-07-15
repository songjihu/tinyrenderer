#include <vector>
#include <cmath>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "iostream"
#include <algorithm>
#include <stdlib.h>
//#include"geometry.cpp"



const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor green = TGAColor(0, 128, 0, 255);

Model* model = NULL;
Model* model_texture = NULL;
const int width = 800;
const int height = 800;
const int depth = 255;
int t_w = 0;
int t_h = 0;

Vec3f camera(0, 0, 3);


int main(int argc, char** argv);

void line(Vec2i t0, Vec2i t1, TGAImage& image, TGAColor color);
void triangle(Vec3i* pts, float* zbuffer, TGAImage& image, TGAColor color, Vec3f* puvs, float intensity);//加入对应点的材质坐标


Vec3f m2v(Matrix m) {
    return Vec3f(m[0][0] / m[3][0], m[1][0] / m[3][0], m[2][0] / m[3][0]);
}

Matrix v2m(Vec3f v) {
    Matrix m;
    m[0][0] = v.x;
    m[1][0] = v.y;
    m[2][0] = v.z;
    m[3][0] = 1.f;
    return m;
}

Matrix viewport(int x, int y, int w, int h) {
    Matrix m = Matrix::identity(4);
    m[0][3] = x + w / 2.f;
    m[1][3] = y + h / 2.f;
    m[2][3] = depth / 2.f;

    m[0][0] = w / 2.f;
    m[1][1] = h / 2.f;
    m[2][2] = depth / 2.f;
    return m;
}


//输入 三角形的3个顶点 待判断点P
Vec3f barycentric(Vec3i* pts, Vec3f P) {
    //求解u v 1
    Vec3f u = cross(Vec3f(pts[2][0] - pts[0][0], pts[1][0] - pts[0][0], pts[0][0] - P[0]), Vec3f(pts[2][1] - pts[0][1], pts[1][1] - pts[0][1], pts[0][1] - P[1]));
    /* `pts` and `P` has integer value as coordinates
       so `abs(u[2])` < 1 means `u[2]` is 0, that means
       triangle is degenerate, in this case return something with negative coordinates */
    if (std::abs(u[2]) < 1) return Vec3f(-1, 1, 1);
    return Vec3f(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
}


int main(int argc, char** argv) {


    if (2 == argc) {
        model = new Model(argv[1]);
    }
    else {
        //model = new Model("obj/african_head/african_head.obj");
        model = new Model("obj/african_head/african_head.obj");
    }

    float* zbuffer = new float[width * height];
    for (int i = 0; i < width * height; i++) {
        zbuffer[i] = -std::numeric_limits<float>::max();
    }

    Matrix Projection = Matrix::identity(4);
    Matrix ViewPort = viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
    Projection[3][2] = -1.f / camera.z;


    TGAImage image(width, height, TGAImage::RGB);
    TGAImage image_t(width, height, TGAImage::RGB);
    Vec3f light_dir(0, 0, -1);
    for (int i = 0; i < model->nfaces(); i++) {
        std::vector<int> face = model->face(i);
        std::vector<int> uvt = model->uvt(i);
        Vec3i screen_coords[3];
        Vec3f world_coords[3];
        Vec3f puvs[3];
        for (int j = 0; j < 3; j++) {
            Vec3f v = model->vert(face[j]);
            Vec2f uv = model->uv(uvt[j]);

            //std::cout << face[j] << uv.x << " " << uv.y << std::endl;

            //screen_coords[j] = Vec3i((v.x + 1.) * width / 2., (v.y + 1.) * height / 2., v.z * 100);
            screen_coords[j] = m2v(ViewPort * Projection * v2m(v));
            world_coords[j] = v;
            puvs[j] = Vec3f(uv.x, uv.y, 0.f);
        }
        Vec3f n = cross((world_coords[2] - world_coords[0]), (world_coords[1] - world_coords[0]));
        //猜想 012所对应的3个坐标有一定的顺序，这导致了模型前后的三角形是不同的
        n.normalize();
        float intensity = n * light_dir;
        if (intensity > 0) {
            Vec3i pts[3] = { screen_coords[0], screen_coords[1], screen_coords[2] };
            //int color_r = 255, color_g = 255, color_b = 255;
            //triangle(pts, zbuffer, image, TGAColor(intensity * color_r, intensity * color_g, intensity * color_b, 255));
            triangle(pts, zbuffer, image, model->diffuse(Vec2f(puvs[0].x, puvs[0].y)), puvs, intensity);
        }

    }
    image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
    image.write_tga_file("output.tga");
    image_t.flip_vertically(); // i want to have the origin at the left bottom corner of the image
    image_t.write_tga_file("output_t.tga");
    delete model;
    return 0;
}


void line(Vec2i t0, Vec2i t1, TGAImage& image, TGAColor color) {
    int x0 = t0.x, y0 = t0.y, x1 = t1.x, y1 = t1.y;
    bool steep = false;
    if (std::abs(x0 - x1) < std::abs(y0 - y1)) {
        std::swap(x0, y0);
        std::swap(x1, y1);
        steep = true;
    }
    if (x0 > x1) {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }
    int dx = x1 - x0;
    int dy = y1 - y0;
    int derror2 = std::abs(dy) * 2;
    int error2 = 0;
    int y = y0;
    for (int x = x0; x <= x1; x++) {
        if (steep) {
            image.set(y, x, color);
        }
        else {
            image.set(x, y, color);
        }
        error2 += derror2;
        if (error2 > dx) {
            y += (y1 > y0 ? 1 : -1);
            error2 -= dx * 2;
        }
    }
}

//加入z坐标
void triangle(Vec3i* pts, float* zbuffer, TGAImage& image, TGAColor color, Vec3f* puvs ,float intensity) {

    if (pts[0].y == pts[1].y && pts[0].y == pts[2].y) return; // i dont care about degenerate triangles

    Vec2i bboxmin(image.get_width() - 1, image.get_height() - 1);
    Vec2i bboxmax(0, 0);
    Vec2i clamp(image.get_width() - 1, image.get_height() - 1);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 2; j++) {
            bboxmin[j] = std::max(0, std::min(bboxmin[j], pts[i][j]));
            bboxmax[j] = std::min(clamp[j], std::max(bboxmax[j], pts[i][j]));
        }
    }

    Vec3f P;
    //遍历bbox中的每一个点
    for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
        for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {
            Vec3f bc_screen = barycentric(pts, P);
            if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0) continue;
            P.z = 0;
            for (int i = 0; i < 3; i++) P.z += pts[i][2] * bc_screen[i];
            if (zbuffer[int(P.x + P.y * width)] < P.z) {
                zbuffer[int(P.x + P.y * width)] = P.z;
                //image.set(P.x, P.y, color);
                //根据比例求出三角形内的纹理坐标
                Vec2f uv = Vec2f(0.f, 0.f);
                for (int j = 0; j < 3; j++) {
                    uv[0] += puvs[j][0] * bc_screen[j];
                    uv[1] += puvs[j][1] * bc_screen[j];
                }
                TGAColor color = model->diffuse(uv);
                //Vec2f uv = Vec2f(puvs[0].x, puvs[0].y)
                image.set(P.x, P.y, TGAColor(color.bgra[2] * intensity, color.bgra[1] * intensity, color.bgra[0] * intensity));
                //image.set(P.x, P.y, color);
            } 
        }
    }
}
