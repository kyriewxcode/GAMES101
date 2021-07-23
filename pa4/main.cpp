#include <chrono>
#include <iostream>
#include <vector>
#include <opencv2/opencv.hpp>

std::vector<cv::Point2f> control_points;

void mouse_handler(int event, int x, int y, int flags, void *userdata) 
{
    if (event == cv::EVENT_LBUTTONDOWN && control_points.size() < 4) 
    {
        std::cout << "Left button of the mouse is clicked - position (" << x << ", "
        << y << ")" << '\n';
        control_points.emplace_back(x, y);
    }     
}

void naive_bezier(const std::vector<cv::Point2f> &points, cv::Mat &window) 
{
    auto &p_0 = points[0];
    auto &p_1 = points[1];
    auto &p_2 = points[2];
    auto &p_3 = points[3];

    for (double t = 0.0; t <= 1.0; t += 0.001) 
    {
        auto point = std::pow(1 - t, 3) * p_0 + 3 * t * std::pow(1 - t, 2) * p_1 +
                 3 * std::pow(t, 2) * (1 - t) * p_2 + std::pow(t, 3) * p_3;

        window.at<cv::Vec3b>(point.y, point.x)[2] = 255;
    }
}

cv::Point2f recursive_bezier(const std::vector<cv::Point2f> &control_points, float t) 
{
    // TODO: Implement de Casteljau's algorithm
    if(control_points.size()==2) return (1-t)*control_points[0]+t*control_points[1];

    std::vector<cv::Point2f> temp;

    for (int i = 0; i < control_points.size()-1; i++)
    {
        temp.push_back((1-t)*control_points[i] + t*control_points[i+1]);
    }

    return recursive_bezier(temp,t);
}

void bezier(const std::vector<cv::Point2f> &control_points, cv::Mat &window) 
{
    // TODO: Iterate through all t = 0 to t = 1 with small steps, and call de Casteljau's 
    // recursive Bezier algorithm.

    for (double t = 0.0; t <= 1.0; t+=0.001)
    {
        auto p = recursive_bezier(control_points,t);
        window.at<cv::Vec3b>(p.y,p.x)[1]=255;

        //Anti-aliasing
        float x = p.x - std::floor(p.x);
        float y = p.y - std::floor(p.y);
        cv::Point2f near;
        cv::Point2f p00;
        cv::Point2f p01;
        cv::Point2f p10;
        cv::Point2f p11;

        float x_flag = x < 0.5f ? -1 : 1;
        float y_flag = y < 0.5f ? -1 : 1;

        p00 = cv::Point2f(std::floor(p.x) + 0.5f, std::floor(p.y) + 0.5f);
        p01 = cv::Point2f(std::floor(p.x + x_flag * 1.0f) + 0.5f, std::floor(p.y) + 0.5f);
        p10 = cv::Point2f(std::floor(p.x) + 0.5f, std::floor(p.y + y_flag * 1.0f) + 0.5f);
        p11 = cv::Point2f(std::floor(p.x + x_flag * 1.0f) + 0.5f, std::floor(p.y + y_flag * 1.0f) + 0.5f);

        std::vector<cv::Point2f> pAA;
        pAA.push_back(p00);
        pAA.push_back(p01);
        pAA.push_back(p10);
        pAA.push_back(p11);

        //calculate distance
        float distance = sqrt((p00-p).x * (p00-p).x + (p00-p).y * (p00-p).y);

        //shading edge
        for(auto point : pAA)
        {
            float d =sqrt((point-p).x * (point-p).x + (point-p).y * (point-p).y);
            float percent = distance/d;

            cv::Vec3d color = window.at<cv::Vec3b>(point.y, point.x);
            color[1] = std::max(color[1], (double)255 * percent);
            window.at<cv::Vec3b>(point.y, point.x) = color;
        }
    }
}

int main()
{
    cv::Mat window = cv::Mat(700, 700, CV_8UC3, cv::Scalar(0));
    cv::cvtColor(window, window, cv::COLOR_BGR2RGB);
    cv::namedWindow("Bezier Curve", cv::WINDOW_AUTOSIZE);

    cv::setMouseCallback("Bezier Curve", mouse_handler, nullptr);

    int key = -1;
    while (key != 27) 
    {
        for (auto &point : control_points) 
        {
            cv::circle(window, point, 3, {255, 255, 255}, 3);
        }

        if (control_points.size() == 4) 
        {
            //naive_bezier(control_points, window);
            bezier(control_points, window);

            cv::imshow("Bezier Curve", window);
            cv::imwrite("my_bezier_curve.png", window);
            key = cv::waitKey(0);

            return 0;
        }

        cv::imshow("Bezier Curve", window);
        key = cv::waitKey(20);
    }

return 0;
}
