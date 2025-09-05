#include <iostream>
//#include <vector>
#include <string>
#include "src/recognition.h"
#include "src/utils.h"

using namespace std;
using namespace cv;

int main() {
    g_debug = true;

    // 使用相对路径读取资源
    Mat image1 = imread("C:\\Project\\ProjectSnow\\images\\11.jpg");
    Mat image2 = imread("C:\\Project\\ProjectSnow\\images\\12.jpg");
    Mat templ = imread("C:\\Project\\ProjectSnow\\images\\template.jpg");
    Mat templ1 = imread("C:\\Project\\ProjectSnow\\images\\template11.jpeg");

    if (image1.empty() || image2.empty() || templ.empty() || templ1.empty()) {
        cerr << "load image failed" << endl;
        return -1;
    }

    // 先为两个模板预计算直方图，供后续相似度比较
    Mat histTempl = Image2Hist(templ);
    Mat histTempl1 = Image2Hist(templ1);

    // 1) image1 与 templ 匹配并裁剪保存
    Point loc1_a = TemplateMatch(image1, templ);
    Rect roi1_a(loc1_a.x, loc1_a.y, templ.cols, templ.rows);
    Mat crop1_a = image1(roi1_a);
    saveImage(crop1_a, "match_image1_with_full.jpg", "matches");

    // 2) image1 与 templ1 匹配并裁剪保存
    Point loc1_b = TemplateMatch(image1, templ1);
    Rect roi1_b(loc1_b.x, loc1_b.y, templ1.cols, templ1.rows);
    Mat crop1_b = image1(roi1_b);
    saveImage(crop1_b, "match_image1_with_templ1.jpg", "matches");

    // 3) image2 与 templ 匹配并裁剪保存
    Point loc2_a = TemplateMatch(image2, templ);
    Rect roi2_a(loc2_a.x, loc2_a.y, templ.cols, templ.rows);
    Mat crop2_a = image2(roi2_a);
    saveImage(crop2_a, "match_image2_with_full.jpg", "matches");

    // 4) image2 与 templ1 匹配并裁剪保存
    Point loc2_b = TemplateMatch(image2, templ1);
    Rect roi2_b(loc2_b.x, loc2_b.y, templ1.cols, templ1.rows);
    Mat crop2_b = image2(roi2_b);
    saveImage(crop2_b, "match_image2_with_templ1.jpg", "matches");

    // 对四个裁剪结果分别与两个模板直方图比较（共8个分数）
    double s1a_t = ImageHistCompare(crop1_a, histTempl);
    double s1a_t1 = ImageHistCompare(crop1_a, histTempl1);
    double s1b_t = ImageHistCompare(crop1_b, histTempl);
    double s1b_t1 = ImageHistCompare(crop1_b, histTempl1);
    double s2a_t = ImageHistCompare(crop2_a, histTempl);
    double s2a_t1 = ImageHistCompare(crop2_a, histTempl1);
    double s2b_t = ImageHistCompare(crop2_b, histTempl);
    double s2b_t1 = ImageHistCompare(crop2_b, histTempl1);

    cout << "image1-full crop vs full: " << s1a_t << endl;
    cout << "image1-full crop vs templ1: " << s1a_t1 << endl;
    cout << "image1-templ1 crop vs full: " << s1b_t << endl;
    cout << "image1-templ1 crop vs templ1: " << s1b_t1 << endl;
    cout << "image2-full crop vs full: " << s2a_t << endl;
    cout << "image2-full crop vs templ1: " << s2a_t1 << endl;
    cout << "image2-templ1 crop vs full: " << s2b_t << endl;
    cout << "image2-templ1 crop vs templ1: " << s2b_t1 << endl;

    return 0;
}