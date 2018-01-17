#include "Armor.h"

Armor::Armor():
    AREA_MAX(200),
    AREA_MIN(25),
    ERODE_KSIZE(2),
    DILATE_KSIZE(4),
    V_THRESHOLD(230),
    S_THRESHOLD(40),
    BORDER_THRESHOLD(10),
    H_BLUE_LOW_THRESHOLD(120),
    H_BLUE_LOW_THRESHOLD_MIN(100),
    H_BLUE_LOW_THRESHOLD_MAX(140),
    H_BLUE_HIGH_THRESHOLD(180),
    H_BLUE_HIGH_THRESHOLD_MAX(200),
    H_BLUE_HIGH_THRESHOLD_MIN(160),
    H_BLUE_STEP(1),
    H_BLUE_CHANGE_THRESHOLD_LOW(5),
    H_BLUE_CHANGE_THRESHOLD_HIGH(10),
    S_BLUE_THRESHOLD(100),
    BLUE_PIXEL_RATIO_THRESHOLD(12),
    CIRCLE_ROI_WIDTH(40),
    CIRCLE_ROI_HEIGHT(40),
    CIRCLE_THRESHOLD(130),
    CIRCLE_AREA_THRESH_MAX(1000),
    CIRCLE_AREA_THRESH_MIN(30),
    DRAW(NO_SHOW),
    is_last_found(false),
    refresh_ctr(1),
    target(cv::Point(320,240)),
    found_num(0),
    V_RATIO(34),
    over_num(0)
{
}

void Armor::init(const cv::Mat& src)
{
    AREA_MAX = 200;
    AREA_MIN = 25;
    ERODE_KSIZE = 2;
    DILATE_KSIZE = 4;
    V_THRESHOLD = 230;
    S_THRESHOLD = 40;
    BORDER_THRESHOLD = 10;
    H_BLUE_LOW_THRESHOLD = 120;
    H_BLUE_LOW_THRESHOLD_MIN = 100;
    H_BLUE_LOW_THRESHOLD_MAX = 140;
    H_BLUE_HIGH_THRESHOLD = 180;
    H_BLUE_HIGH_THRESHOLD_MAX = 200;
    H_BLUE_HIGH_THRESHOLD_MIN = 160;
    H_BLUE_STEP = 1;
    H_BLUE_CHANGE_THRESHOLD_LOW = 5;
    H_BLUE_CHANGE_THRESHOLD_HIGH = 10;
    S_BLUE_THRESHOLD = 100;
    BLUE_PIXEL_RATIO_THRESHOLD = 12;
    CIRCLE_ROI_WIDTH = 40;
    CIRCLE_ROI_HEIGHT = 40;
    CIRCLE_THRESHOLD = 130;
    CIRCLE_AREA_THRESH_MAX = 1000;
    CIRCLE_AREA_THRESH_MIN = 30;
    target = cv::Point(320,240);
    is_last_found = false;
    refresh_ctr = 0;
    found_num = 0;
    over_num = 0;
    //frame_num = 0;
    V_element_erode = cv::getStructuringElement(
            cv::MORPH_CROSS, cv::Size(ERODE_KSIZE, ERODE_KSIZE));
    V_element_dilate = cv::getStructuringElement(
            cv::MORPH_CROSS, cv::Size(DILATE_KSIZE, DILATE_KSIZE));
    getSrcSize(src);
    if(DRAW & SHOW_DRAW)
    {
        cout << "Show draw!" << endl;
        cv::namedWindow("draw",1);
    }
    if(DRAW & SHOW_GRAY)
    {
        cout << "Show gray!" << endl;
        cv::namedWindow("gray",1);
    }
    if(DRAW & SHOW_LIGHT_REGION)
    {
        cout << "Show light region" << endl;
        cv::namedWindow("light region",1);
    }
}

void Armor::feedImage(const cv::Mat& src)
{
    static double fps;
    static vector<cv::Mat > hsvSplit;
    static cv::Mat v_very_high;
    fps = tic();
    if(DRAW & SHOW_DRAW)
        light_draw = src.clone();
//    if(is_last_found)
//    {
//        ++refresh_ctr;
//        findCircleAround(src);
//        cout << "FPS:" << 1/(tic() - fps) << endl;
//        if(refresh_ctr == 500)
//            is_last_found = false;
//        return;
//    }
    cleanAll();
    cvtHSV(src, hsvSplit);
    showhist(hsvSplit);
    getLightRegion(hsvSplit, v_very_high);
    findContours(v_very_high, V_contours,
            CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);
    selectContours(hsvSplit);
    selectLights(src);
    is_last_found = isFound();
    if(is_last_found)found_num = found_num + 1;
    cout << "50% frame num" << dec << over_num << endl;
	//cout << "found circle number" << dec << found_num<< endl;
    cout << "FPS:" << 1/(tic() - fps) << endl;
    fps = tic();
}

void Armor::getSrcSize(const cv::Mat& src)
{
    srcH = (int)src.size().height;
    srcW = (int)src.size().width;
}

void Armor::cvtHSV(const cv::Mat& src,vector<cv::Mat >& hsvSplit)
{
    static cv::Mat hsv;
    cv::cvtColor(src, hsv, CV_BGR2HSV_FULL);
    cv::split(hsv, hsvSplit);
}

void Armor::cvtGray(const cv::Mat& src, cv::Mat& gray)
{
    cv::cvtColor(src, gray, CV_BGR2GRAY);
    cv::equalizeHist(gray, gray);
    cv::threshold(gray, gray, CIRCLE_THRESHOLD, U8_MAX, cv::THRESH_BINARY);
    if(DRAW & SHOW_GRAY)
    {
        cv::imshow("gray", gray);
        cv::createTrackbar("THRESHOLD", "gray", &CIRCLE_THRESHOLD, U8_MAX);
    }
}

void Armor::getLightRegion(vector<cv::Mat >& hsvSplit, cv::Mat& v_very_high)
{
    static cv::Mat s_low;
    cv::threshold(hsvSplit[V_INDEX], v_very_high,
            V_THRESHOLD, U8_MAX, cv::THRESH_BINARY);
    cv::threshold(hsvSplit[S_INDEX], s_low,
            S_THRESHOLD, U8_MAX, cv::THRESH_BINARY_INV);
    bitwise_and(s_low, v_very_high, v_very_high);
    cv::erode(v_very_high, v_very_high, V_element_erode);
    cv::dilate(v_very_high, v_very_high, V_element_dilate);
    if(DRAW & SHOW_LIGHT_REGION)
    {
        cout << "light region" << endl;
        cv::imshow("light region", v_very_high);
        //cv::imshow("S region", s_low);
        cv::createTrackbar("V_THRESHOLD", "light region", &V_THRESHOLD, U8_MAX);
        cv::createTrackbar("S_THRESHOLD", "light region", &S_THRESHOLD, U8_MAX);
    }
}

void Armor::selectContours(vector<cv::Mat >& hsvSplit)
{
        cv::RotatedRect rotated_rect;
        for (int i = 0; i < (int)V_contours.size(); i++)
        {
            if(isAreaTooBigOrSmall(V_contours[i]))
                continue;
            rotated_rect = minAreaRect(V_contours[i]);
            if(isCloseToBorder(rotated_rect))
                continue;
            if(!isBlueNearby(hsvSplit, V_contours[i]))
                continue;
	    //  cout << "Found Light!" << endl;
            lights.push_back(rotated_rect);//确认为灯条

            if(DRAW & SHOW_DRAW)
            {
                cv::Point2f vertices[4];
                rotated_rect.points(vertices);
                for (int j = 0; j < 4; ++j)
                    line(light_draw, vertices[j], vertices[(j + 1) % 4], cv::Scalar(0, 255, 0), 2);

                cv::imshow("draw", light_draw);
            }
        }
}


bool Armor::isAreaTooBigOrSmall(vector<cv::Point>& contour)//将灯条的范围限制在合理区域内
{
    int area = contourArea(contour);
    return (area > AREA_MAX || area < AREA_MIN);//如果灯条的体积太小，应该是噪音，如果太大，则应该是大的面积的反光
}

bool Armor::isCloseToBorder(cv::RotatedRect& rotated_rect)
{
            double recx = rotated_rect.center.x;
            double recy = rotated_rect.center.y;
            return (recx < BORDER_THRESHOLD || recx > srcW - BORDER_THRESHOLD
                    || recy < BORDER_THRESHOLD
                    || recy > srcH - BORDER_THRESHOLD);

}

bool Armor::isBlueNearby(vector<cv::Mat >& hsvSplit, vector<cv::Point>& contour)
{
    int blue_pixel_cnt = 0;
    uchar pixel = 0;//还在困扰pixel究竟是什么？
    uchar pixel_min = H_BLUE_LOW_THRESHOLD_MAX;
    uchar pixel_max = H_BLUE_HIGH_THRESHOLD_MIN;
    uchar pixel_S_select = 0;
    for(int j=0; j < (int)contour.size(); ++j)
    {
        pixel_S_select = *(hsvSplit[S_INDEX].ptr<uchar>(contour[j].y) + contour[j].x);
        pixel = *(hsvSplit[H_INDEX].ptr<uchar>(contour[j].y) + contour[j].x);
        if(pixel_S_select > S_BLUE_THRESHOLD && pixel < H_BLUE_HIGH_THRESHOLD && pixel > H_BLUE_LOW_THRESHOLD)
        {
            //cout << "pixel S:" << (int)pixel_S_select << " blue:" << (int)pixel << endl;
            if(pixel > pixel_max)
                pixel_max = pixel;
            if(pixel < pixel_min)
                pixel_min = pixel;//找出目前最低的蓝色像素值
            ++blue_pixel_cnt;
        }
    }
    if( blue_pixel_cnt > 5 )//这块是不断更新blue的threshold
    {
        pixel_max = pixel_max < H_BLUE_HIGH_THRESHOLD_MAX ? pixel_max:H_BLUE_HIGH_THRESHOLD_MAX;
        pixel_max = pixel_max > H_BLUE_HIGH_THRESHOLD_MIN ? pixel_max:H_BLUE_HIGH_THRESHOLD_MIN;
        pixel_min = pixel_min > H_BLUE_LOW_THRESHOLD_MIN ? pixel_min:H_BLUE_LOW_THRESHOLD_MIN;
        pixel_min = pixel_min < H_BLUE_LOW_THRESHOLD_MAX ? pixel_min:H_BLUE_LOW_THRESHOLD_MAX;
        if(pixel_min > H_BLUE_LOW_THRESHOLD + H_BLUE_CHANGE_THRESHOLD_HIGH)
            H_BLUE_LOW_THRESHOLD += H_BLUE_STEP;
        if(pixel_min < H_BLUE_LOW_THRESHOLD + H_BLUE_CHANGE_THRESHOLD_LOW)
            H_BLUE_LOW_THRESHOLD -= H_BLUE_STEP;
        if(pixel_max < H_BLUE_HIGH_THRESHOLD - H_BLUE_CHANGE_THRESHOLD_HIGH)
            H_BLUE_HIGH_THRESHOLD -= H_BLUE_STEP;
        if(pixel_max > H_BLUE_HIGH_THRESHOLD - H_BLUE_CHANGE_THRESHOLD_LOW)
            H_BLUE_HIGH_THRESHOLD += H_BLUE_STEP;
    }
    if(blue_pixel_cnt > 5) cout << "blue ratio: "<<(float)blue_pixel_cnt/contour.size()*100 << "%" << endl;
    //cout << "fuck me!" << (float) blue_pixel_cnt/contour.size()*100 << endl;
    if( blue_pixel_cnt*100/contour.size() > 50) over_num ++;

    return float(blue_pixel_cnt)/contour.size()*100 > BLUE_PIXEL_RATIO_THRESHOLD;
}

void Armor::selectLights(const cv::Mat& src)
{
    if (lights.size() <= 1)
        return;
    cv::Point2f pi;
    cv::Point2f pj;
    double midx;
    double midy;
    cv::Size2f sizei;
    //Size2f sizej = lights.at(j).size;
    double ai;
    //double b=sizei.height<sizei.width?sizei.height:sizei.width;
    double distance;
    static cv::Mat gray;
    cvtGray(src, gray);
    for (int i = 0; i < (int)lights.size() - 1; i++)
    {
        for (int j = i + 1; j < (int)lights.size(); j++)
        {
            pi = lights.at(i).center;
            pj = lights.at(j).center;
            midx = (pi.x + pj.x) / 2;
            midy = (pi.y + pj.y) / 2;//遍历所有的矩形类，然后去找这些矩形类里面中间有没有圆
            sizei = lights.at(i).size;
            ai = sizei.height > sizei.width ? sizei.height : sizei.width;
            distance = sqrt((pi.x - pj.x) * (pi.x - pj.x) + (pi.y - pj.y) * (pi.y - pj.y));

            CIRCLE_ROI_WIDTH = int(distance * 1.5);
            CIRCLE_ROI_HEIGHT = int(ai * 3);
            //灯条距离合适
            if (distance < 1.5 * ai || distance > 7.5 * ai)//这两个是怎么一回事呢？
                continue;
            if(!isCircleAround(gray, midx, midy))//找到圆了就可以继续了
                continue;
        }
    }

    chooseCloseTarget();

    if(DRAW & SHOW_DRAW)//下面这里是debug用，画出圆圈
    {
        circle(light_draw, target, 3, cv::Scalar(0,0,255), -1);
        imshow("draw", light_draw);
        cv::createTrackbar("H_BLUE_LOW_THRESHOLD", "draw", &H_BLUE_LOW_THRESHOLD, U8_MAX);
        cv::createTrackbar("H_BLUE_HIGH_THRESHOLD", "draw", &H_BLUE_HIGH_THRESHOLD, U8_MAX);
        cv::createTrackbar("S_BLUE_THRESHOLD", "draw", &S_BLUE_THRESHOLD, U8_MAX);
        cv::createTrackbar("BLUE_PIXEL_RATIO_THRESHOLD", "draw", &BLUE_PIXEL_RATIO_THRESHOLD, 100);
    }
}

void Armor::chooseCloseTarget()
{
    if(armors.empty())//armor就是一堆点,一堆灯条中间的点
        return;
    int closest_x = 0, closest_y = 0;
    int distance_armor_center = 0;
    int distance_last = sqrt(
            (closest_x - srcW/2) * (closest_x - srcW/2)
          + (closest_y - srcH/2) * (closest_y - srcH/2));
    for(int i=0; i<(int)armors.size(); ++i)//这里我在推测armors这个类的作用
    {
        //cout << "x:" << armors[i].x
        //<< "y:" << armors[i].y << endl;
        distance_armor_center = sqrt(
                (armors[i].x - srcW/2) * (armors[i].x - srcW/2)
                + (armors[i].y - srcH/2) * (armors[i].y - srcH/2));
        if(distance_armor_center < distance_last)
        {
            closest_x = (int)armors[i].x;
            closest_y = (int)armors[i].y;
            distance_last = distance_armor_center;
        }
    }
    target.x = closest_x;
    target.y = closest_y;
}

bool Armor::isCircleAround(cv::Mat& gray, int midx, int midy)
{
    if(midx - CIRCLE_ROI_WIDTH < 0
            || midy - CIRCLE_ROI_HEIGHT < 0
            || midx + CIRCLE_ROI_WIDTH > srcW
            || midy + CIRCLE_ROI_HEIGHT > srcH)
        return false;
    cv::Mat roi_circle =
        gray(
                cv::Rect(
                    midx - CIRCLE_ROI_WIDTH/2,
                    midy - CIRCLE_ROI_HEIGHT/2,
                    CIRCLE_ROI_WIDTH, CIRCLE_ROI_HEIGHT)
            );
    vector<vector<cv::Point> > gray_contours;
    if(DRAW & SHOW_ROI)
        cv::imshow("roi", roi_circle);
    cv::findContours(roi_circle.clone(), gray_contours, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);

    int area;
    cv::Point2f center(320, 240);
    float radius;
    float circleArea;
    float r;
    for(int i= 0; i < (int)gray_contours.size(); i++)
    {
        area= contourArea(gray_contours[i]);//确定圆形图案的边界
        if(area < 30)//30是什么magic number？
            continue;
        cv::minEnclosingCircle(gray_contours[i], center, radius);
        center.x += midx - CIRCLE_ROI_WIDTH/2;
        center.y += midy - CIRCLE_ROI_HEIGHT/2;
        circleArea = PI * radius * radius;
        r = area / circleArea;//这个比例有什么意义的？
        //cout << "Circle:" << r << endl;
        if(r > 0.7)
        {

            if(DRAW & SHOW_DRAW)
                cv::circle(light_draw, center, radius, cv::Scalar(0, 255, 255), 2);
            armors.push_back(center);//center是什么呢？

            return true;
        }
    }
    return false;
}

void Armor::cleanAll()
{
    V_contours.clear();
    lights.clear();
    armors.clear();
}

bool Armor::isFound()
{
    return !armors.empty();
}

int Armor::getTargetX()
{
    return target.x;
}

int Armor::getTargetY()
{
    return target.y;
}

void Armor::setDraw(int is_draw)
{
    DRAW = is_draw;
}

void Armor::findCircleAround(const cv::Mat& src)
{
    cv::Mat roi_possible_circle =
        src(
            cv::Rect(
                target.x - CIRCLE_ROI_WIDTH/2,
                target.y - CIRCLE_ROI_HEIGHT/2,
                CIRCLE_ROI_WIDTH,
                CIRCLE_ROI_HEIGHT)
            );

    cv::cvtColor(roi_possible_circle, roi_possible_circle, CV_BGR2GRAY);
    cv::equalizeHist(roi_possible_circle, roi_possible_circle);
    cv::threshold(roi_possible_circle, roi_possible_circle, CIRCLE_THRESHOLD, U8_MAX, cv::THRESH_BINARY);

    if(DRAW & SHOW_GRAY)
    {
        cv::imshow("gray", roi_possible_circle);
        cv::createTrackbar("THRESHOLD", "gray", &CIRCLE_THRESHOLD, U8_MAX);
    }

    if(DRAW & SHOW_ROI)
        cv::imshow("roi", roi_possible_circle);

    vector<vector<cv::Point> > possible_circle_contours;
    cv::findContours(roi_possible_circle, possible_circle_contours, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);
    cv::Point2f center(320, 240);
    float radius = 0;
    int area;
    float circleArea;
    float r;
    for(int i= 0; i < (int)possible_circle_contours.size(); i++)
    {
        area = cv::contourArea(possible_circle_contours[i]);
        //cout << "Area:" << area << endl;
        if(area > CIRCLE_AREA_THRESH_MAX)
            continue;
        if(area < CIRCLE_AREA_THRESH_MIN)
            continue;
        if(area < 30)
            continue;
        cv::minEnclosingCircle(possible_circle_contours[i], center, radius);
        center.x += target.x - CIRCLE_ROI_WIDTH/2;
        center.y += target.y - CIRCLE_ROI_HEIGHT/2;
        circleArea= PI * radius * radius;
        r= area / circleArea;
            //cout << "Circle:" << r << endl;
        if(r > 0.7)
        {
            CIRCLE_AREA_THRESH_MAX = circleArea * 3;
            CIRCLE_AREA_THRESH_MIN = circleArea / 3;
            //cout << "Max:" << CIRCLE_AREA_THRESH_MAX;
            //cout << "Min:" << CIRCLE_AREA_THRESH_MIN;
            CIRCLE_ROI_WIDTH = radius * 8;
            CIRCLE_ROI_HEIGHT = radius * 4;
            target = center;
            is_last_found = true;
            if(DRAW & SHOW_DRAW)
            {
                cv::circle(light_draw, center, radius, cv::Scalar(0, 255, 255), 2);
                circle(light_draw, target, 3, cv::Scalar(0,0,255), -1);
                imshow("draw", light_draw);
                cv::createTrackbar("H_BLUE_LOW_THRESHOLD", "draw", &H_BLUE_LOW_THRESHOLD, U8_MAX);
                cv::createTrackbar("H_BLUE_HIGH_THRESHOLD", "draw", &H_BLUE_HIGH_THRESHOLD, U8_MAX);
                cv::createTrackbar("S_BLUE_THRESHOLD", "draw", &S_BLUE_THRESHOLD, U8_MAX);
                cv::createTrackbar("BLUE_PIXEL_RATIO_THRESHOLD", "draw", &BLUE_PIXEL_RATIO_THRESHOLD, 100);
            }
            return;
        }
    }
    CIRCLE_ROI_WIDTH = 1000;
    CIRCLE_ROI_HEIGHT = 30;
    is_last_found = false;
}

double Armor::tic()
{
    struct timeval t;
    gettimeofday(&t, NULL);
    return ((double)t.tv_sec + ((double)t.tv_usec) / 1000000.);
}

void Armor::showhist(vector<cv::Mat> hsvSplit)
{
	cv::Mat V_hist, V_histImg;
	int histSize = 256;
	float range[] = { 0, 256 };
	const float* histRange = { range };
	int channels[] = {0};
	bool uniform = true;
	bool accumulate = false;

	/*计算直方图*/
	cv::calcHist(&hsvSplit[V_INDEX], 1, channels, cv::Mat(), V_hist, 1, &histSize, &histRange, uniform, accumulate);
//    cv::calcHist(&hsvSplit[S_INDEX], 1, channels, cv::Mat(), S_hist, 1, &histSize, &histRange, uniform, accumulate);

	//一个点之间的距离，宽度是h，那么把宽度分成了256份

    cv::Mat V_histImage(100, 256, CV_8UC3, cv::Scalar(0, 0, 0));
	/*直方图归一化范围，histImage.rows]*/
	cv::normalize(V_hist, V_hist, 100, 0, cv::NORM_MINMAX, -1, cv::Mat());
	/*画直线*/
	for (int i = 1; i < histSize; ++i)
	{
//		cvRound：类型转换。 这里hist为256*1的一维矩阵，存储的是图像中各个灰度级的归一化值
        cv::line(V_histImage,cv::Point(i - 1, 100 - cvRound(V_hist.at<float>(i - 1))),//（x,y）坐标
			 cv::Point(i, 100 - cvRound(V_hist.at<float>(i))),
			 cv::Scalar(0, 0, 255), 2, 8, 0);
	}
    cv::line(V_histImage,cv::Point(V_THRESHOLD,0),cv::Point(V_THRESHOLD,100),cv::Scalar(255, 255, 255), 2, 8, 0);

    if (V_hist.isContinuous())
	{
		float * matpt = V_hist.ptr<float>(0);
		float sum_hist = 0;
		for (int i = 0; i < 256; i++)
		{
			sum_hist += *(matpt+i);
//			cout<< *(matpt+i) << " this many pixels belong to the value: "<< i << endl;
		}
		cout << "The proper hist would be" << sum_hist << endl;
		float proper_hist = sum_hist* V_RATIO;
		float temp_hist = 0;

		for (int i = 255; i > -1; i--)
		{
			temp_hist += *(matpt+i) * 1000;
			if (proper_hist <= temp_hist)
			{
				V_THRESHOLD = i;
				break;
			}
		}

	}

	cv::namedWindow("V_Channel",1);
	cv::namedWindow("V_hist",1);
	cv::imshow("V_Channel", hsvSplit[V_INDEX]);
	cv::imshow("V_hist", V_histImage);
	cv::createTrackbar("V_RATIO", "V_Channel", &V_RATIO, 1000);
	cv::waitKey(0);
}
