#include <opencv2/opencv.hpp>
#include "FaceTracker.h"
#include "StrCommon.h"
#ifdef WITHDETECT
#include "/home/xyz/code1/FaceTracker/detectalign/Detector.h"
#endif
#include "Util.h"
#include "VLCCore.h"


FaceTracker *_tt = NULL;


#ifdef WITHDETECT
//boost::shared_ptr<FaceAlign> FaceAlign::self_;
#endif

std::string RPATH = "";
std::string FFILE = "";

bool _isShow = false;
void DrawData(cv::Mat mm, cv::Mat frame, const std::map<int, DSResult> &map, 
		const std::vector<cv::Rect> &outRcs,
		bool detect,
		int num){
	std::map<int, DSResult>::const_iterator it;
	for(it = map.begin(); it != map.end(); ++it){
		CvScalar clr = cvScalar(0, 255, 0);
		cv::Rect rc = it->second.rc_;
		int oriPos = it->second.oriPos_;
    		cv::rectangle(frame, rc, clr);
		std::string disp = toStr(it->first);
		disp += "---oriPos:(";
		disp += toStr(oriPos);
		disp += ")";
		cv::putText(frame, 
			disp, 
			cvPoint(rc.x, rc.y), 
			CV_FONT_HERSHEY_SIMPLEX, 
			0.6, 
			cv::Scalar(0, 0, 255));
	}
	//
	CvScalar clr = cvScalar(0, 0, 255);
	for(cv::Rect rc:outRcs){
		cv::rectangle(mm, rc, clr);	
		if(detect){
			std::string disp = "detect";
			cv::putText(mm, 
				disp, 
				cvPoint(100, 100), 
				CV_FONT_HERSHEY_SIMPLEX, 
				1, 
				cv::Scalar(0, 0, 255));
			cv::putText(frame, 
				disp, 
				cvPoint(100, 100), 
				CV_FONT_HERSHEY_SIMPLEX, 
				1, 
				cv::Scalar(0, 0, 255));
		}
	}
}

void DrawData2d(cv::Mat mm, cv::Mat frame, const std::map<int, DSResult> &map, 
		const std::vector<cv::Rect> &outRcs,
		bool detect,
		const std::vector<cv::Rect2d> &drs,
		int num){
	std::map<int, DSResult>::const_iterator it;
	LOG(ERROR) << "map.size()" << map.size();
	for(it = map.begin(); it != map.end(); ++it){
		CvScalar clr = cvScalar(0, 255, 0);
		cv::Rect rc = it->second.rc_;
		LOG(ERROR) << "rc(" << rc.x << "," << rc.y << "," << rc.width << "," << rc.height << ")";
		int oriPos = it->second.oriPos_;
    		cv::rectangle(frame, rc, clr);
		std::string disp = toStr(it->first);
		disp += "---oriPos:(";
		disp += toStr(oriPos);
		disp += ")";
		cv::putText(frame, 
			disp, 
			cvPoint(rc.x, rc.y), 
			CV_FONT_HERSHEY_SIMPLEX, 
			0.6, 
			cv::Scalar(0, 0, 255));
	}
	//
	CvScalar clr = cvScalar(0, 0, 255);
	for(cv::Rect rc:outRcs){
		cv::rectangle(mm, rc, clr);	
		if(detect){
			std::string disp = "detect";
			cv::putText(mm, 
				disp, 
				cvPoint(100, 100), 
				CV_FONT_HERSHEY_SIMPLEX, 
				1, 
				cv::Scalar(0, 0, 255));
			cv::putText(frame, 
				disp, 
				cvPoint(100, 100), 
				CV_FONT_HERSHEY_SIMPLEX, 
				1, 
				cv::Scalar(0, 0, 255));
		}
	}
}


void ReadFileContent(const std::string &file, std::string &content){
	FILE *fl = fopen(file.c_str(), "rb");
	if(fl == NULL){
		return;
	}
	fseek(fl, 0, SEEK_END);
	int len = ftell(fl);
	if(len <= 0){
		return;
	}
	fseek(fl, 0, SEEK_SET);
	char *buf = new char[len+1];
	memset(buf, 0, len+1);
	fread(buf, 1, len, fl);
	content = std::string(buf);
	delete []buf;
	fclose(fl);
}

std::map<int, std::vector<cv::Rect>> _rcMap;
void ReadRcFileTotal(const std::string &file) {
	std::string content = "";
	ReadFileContent(file, content);


	std::vector<std::string> lines;
	splitStr(content, "\n", lines);
	std::vector<cv::Rect> rcs;
	int num = -1;
	int tmpNum = -1;
	for (int i = 0; i < lines.size(); i++) {
		std::vector<std::string> cols;
		splitStr(lines[i], ",", cols);
		if (cols.size() < 6) {
			continue;
		}
		tmpNum = toInt(trim(cols[0]));
		if (num!=-1 && tmpNum!=num) {
			_rcMap.insert(std::make_pair(num, rcs));
			rcs.clear();
			num = tmpNum;
		}
		if (num == -1) {
			num = tmpNum;
		}
		cv::Rect rc;
		rc.x = toInt(trim(cols[2]));
		rc.y = toInt(trim(cols[3]));
		rc.width = toInt(trim(cols[4]));
		rc.height = toInt(trim(cols[5]));
		rcs.push_back(rc);
	}
	if (!rcs.empty()) {
		_rcMap.insert(std::make_pair(tmpNum, rcs));
	}
}
std::string _rcFile = "";
std::string _imgDir;
cv::VideoWriter *_vw = NULL;
int _imgCount = 0;


	std::vector<cv::Rect> toTrackRc(int ww, int hh, std::vector<cv::Rect> &rcs){
		std::vector<cv::Rect> re;
		for(int i = 0; i < rcs.size(); i++){
			cv::Rect rc = rcs[i];

			float w = (float)rc.width/3;
			float h = (float)rc.height/6;
			cv::Rect tmp = rc;
			tmp.x -= w; if(tmp.x < 0)tmp.x = 0;
			tmp.y -= h; if(tmp.y <0)tmp.y = 0;
			tmp.width += 2*w; if(tmp.x+tmp.width > ww-1)tmp.width=ww-1-tmp.x;
			tmp.height += 2*h; if(tmp.y+tmp.height > hh-1)tmp.height=hh-1-tmp.y;
			re.push_back(tmp);
		}
		return re;
	}

void CB(cv::Mat &frame, int num){
	if (_vw == NULL) {
		_vw = new cv::VideoWriter("out.avi", CV_FOURCC('M', 'J', 'P', 'G'), 25.0, cv::Size(frame.cols, frame.rows));
	}
	if (_rcMap.empty()) {
		ReadRcFileTotal(_rcFile);
	}
	std::vector<cv::Rect> rcs;
#ifndef WITHDETECT
//#if 0
	std::map<int, std::vector<cv::Rect>>::iterator it = _rcMap.find(num);
	std::vector<int> oriPosVec;
	if (it != _rcMap.end()) {
		rcs = it->second;
		for(int i = 0; i < rcs.size(); i++){
			oriPosVec.push_back(i);
		}
	}
#else
	std::vector<int> oriPosVec;
	int64_t dtm1 = gtm();
	std::vector<cv::Rect2d> rcs2d;
	if(num%90 == 0){
		rcs2d  = Detector::Instance()->Detect2d(frame);
	}
	for(cv::Rect2d tmp: rcs2d){
		cv::Rect rr;
		rr.x = tmp.x;
		rr.y = tmp.y;
		rr.width = tmp.width;	
		rr.height = tmp.height;
		rcs.push_back(rr);
	}
	int64_t dtm2 = gtm();
	for(int i = 0; i < rcs.size(); i++){
		oriPosVec.push_back(i);
	}
#endif
	std::vector<cv::Rect> outRcs;
	
	int64_t tm0 = gtm();
	std::vector<cv::Rect> trackRcs = rcs;
	std::map<int, DSResult> map = _tt->UpdateAndGet(frame, trackRcs, num, outRcs, oriPosVec);
	int64_t tm1 = gtm();
	Mat mm = frame.clone();
	static Mat oo;
	static bool resize = false;
	if(!rcs.empty()){
		oo  = frame.clone();
		resize = false;
	}
	bool detect = (!rcs.empty());
	DrawData2d(mm, frame, map, outRcs, detect, rcs2d, num);
	LOG(ERROR) << "finish " << num << " frame, detectcasttime:" << (dtm2-dtm1) << ", updatecasttime:" << (tm1-tm0);

	//(*_vw) << frame;
	// ooo
	for(int i = 0; i < rcs.size(); i++){
		Rect rc = rcs[i];
		CvScalar clr = cvScalar(255, 0, 0);
    		cv::rectangle(oo, rc, clr);
		std::string disp = toStr(i);
		cv::putText(oo, 
			disp, 
			cvPoint(rc.x, rc.y), 
			CV_FONT_HERSHEY_SIMPLEX, 
			0.8, 
			cv::Scalar(0, 0, 255));
	}
	if(_isShow){
		cv::resize(mm, mm, cv::Size(mm.cols/2, mm.rows/2));
		cv::resize(frame, frame, cv::Size(frame.cols/2, frame.rows/2));
		if(!oo.empty() && (!resize) ){
			cv::resize(oo, oo, cv::Size(oo.cols/2, oo.rows/2));
			resize = true;
		}
		cv::imshow("mm", mm);
		cv::imshow("frame", frame);
		if(!oo.empty()){
			cv::imshow("oo", oo);
		}
		cv::waitKey(2);
	}

}

void Go() {
#if 0
	std::string root = _imgDir;
	for (int i = 1; i < _imgCount; i++) {
		std::string path = root;
		path += to6dStr(i);
		path += ".jpg";
		cv::Mat mat = cv::imread(path);
		CB(mat, i);
		if(i >= _imgCount-1){
			i = 0;
		}	
	}
#else
	std::string path = FFILE;//"/home/xyz/test/construction.mp4";
	RPATH = "/home/xyz/code/test3/BestFrameData/";
	RPATH += getFileName(path);
	VLCCore::Init();
	//VLCCore vlc("/home/xyz/test/g20.mp4", CB);
	//VLCCore vlc("/home/xyz/result/0821_1.mp4", CB);
	VLCCore vlc(path, CB);
	vlc.Start();	
#endif
}


int main(int argc, char **argv){
	if (argc < 3) {
		printf("usage:\n./tt showornot(0/1) file\n");
		return 0;
	}
	std::string pre = "./log/";

	FLAGS_log_dir = pre.c_str();
	google::InitGoogleLogging(argv[0]);

	std::string path = pre;
	path += "INFO_";
	google::SetLogDestination(google::GLOG_INFO, path.c_str());

	path = pre;
	path += "ERROR_";
	google::SetLogDestination(google::GLOG_ERROR, path.c_str());

	path = pre;
	path += "WARNING_";
	google::SetLogDestination(google::GLOG_WARNING, path.c_str());
	FLAGS_colorlogtostderr = true;
	FLAGS_logbufsecs = 0;
	FLAGS_max_log_size = 1024;
	FLAGS_stop_logging_if_full_disk = true;
	FLAGS_v = -1;

	_isShow = toInt(argv[1]);
	FFILE = argv[2];

#ifdef WITHDETECT
	//FaceDetect::Instance()->Init();
	Detector::Instance()->Init();
	//FaceAlign::Instance()->Init();
#endif

	_tt = new FaceTracker();
	if(!_tt->Init()){
		return 0;
	}


	_imgDir = "/home/deepir/code1/GEP/FrameBuffer/imglog/img1/";
	_rcFile = "/home/deepir/code1/GEP/FrameBuffer/imglog/det/det.txt";
	
	_imgCount = 650;// 2001;// 750;// 680;
	Go();
	return 0;
}
