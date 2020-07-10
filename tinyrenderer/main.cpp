﻿#include <vector>
#include <cmath>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "iostream"
#include <algorithm>

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor green = TGAColor(0, 128, 0, 255); 
const TGAColor blue = TGAColor(0, 0, 255, 255);

Model* model = NULL;
const int width = 800;
const int height = 800;


void line(Vec2i t0, Vec2i t1, TGAImage& image, TGAColor color);
void triangle(Vec2i* pts, TGAImage& image, TGAColor color) ;

//输入 三角形的3个顶点 待判断点P
Vec3f barycentric(Vec2i* pts, Vec2i P) {
    //求解u v 1
    Vec3f u = cross(Vec3f(pts[2][0] - pts[0][0], pts[1][0] - pts[0][0], pts[0][0] - P[0]), Vec3f(pts[2][1] - pts[0][1], pts[1][1] - pts[0][1], pts[0][1] - P[1]));
    /* `pts` and `P` has integer value as coordinates
       so `abs(u[2])` < 1 means `u[2]` is 0, that means
       triangle is degenerate, in this case return something with negative coordinates */
    if (std::abs(u[2]) < 1) return Vec3f(-1, 1, 1);
    return Vec3f(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
}

void rasterize(Vec2i p0, Vec2i p1, TGAImage& image, TGAColor color, int ybuffer[]) {
    if (p0.x > p1.x) {
        std::swap(p0, p1);//确保从左到右
    }
    for (int x = p0.x; x <= p1.x; x++) {
        float t = (x - p0.x) / (float)(p1.x - p0.x);//从左到右移动的比例
        int y = p0.y * (1. - t) + p1.y * t;//计算距离y
        if (ybuffer[x] < y) {
            ybuffer[x] = y;//用最近的取代
            for(int i=0;i<16;i++)image.set(x, i, color);
        }
    }
}

int main(int argc, char** argv) {



    TGAImage render(width, 16, TGAImage::RGB);
    int ybuffer[width];
    for (int i = 0; i < width; i++) {
        ybuffer[i] = std::numeric_limits<int>::min();//填充为最小
    }
    rasterize(Vec2i(20, 34), Vec2i(744, 400), render, red, ybuffer);
    rasterize(Vec2i(120, 434), Vec2i(444, 400), render, green, ybuffer);
    rasterize(Vec2i(330, 463), Vec2i(594, 200), render, blue, ybuffer);

    render.flip_vertically(); // i want to have the origin at the left bottom corner of the image
    render.write_tga_file("render.tga");
    delete model;
    return 0;
}


void line(Vec2i t0, Vec2i t1,TGAImage& image, TGAColor color) {
    int x0=t0.x, y0=t0.y, x1=t1.x, y1=t1.y;
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

void triangle(Vec2i* pts, TGAImage& image, TGAColor color) {
    Vec2i bboxmin(image.get_width() - 1, image.get_height() - 1);
    Vec2i bboxmax(0, 0);
    Vec2i clamp(image.get_width() - 1, image.get_height() - 1);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 2; j++) {
            bboxmin[j] = std::max(0, std::min(bboxmin[j], pts[i][j]));
            bboxmax[j] = std::min(clamp[j], std::max(bboxmax[j], pts[i][j]));
        }
    }
    Vec2i P;
    //遍历bbox中的每一个点
    for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
        for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {
            Vec3f bc_screen = barycentric(pts, P);
            if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0) continue;
            image.set(P.x, P.y, color);
        }
    }
}