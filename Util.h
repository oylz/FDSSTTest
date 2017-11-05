#ifndef _UTILH_
#define _UTILH_


#include <stdio.h>
#include <opencv2/opencv.hpp>
 #define GLOG_NO_ABBREVIATED_SEVERITIES
#include <glog/logging.h>
#include <sys/time.h>

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>


using namespace cv;
#ifdef OPENCV24
typedef Rect Rect2d;
#endif

struct DTM {
	static bool used_;
	static bool uset_;
	static bool usem_;
	static double angley_;
	static double anglep_;
	static double angler_;
	static double blur_;
	static bool imglog_;
	static int edgeMargin_;
	static int trackParalCount_;
	static std::string cameraUser_;
	static std::string cameraPwd_;
};


static std::string toStr(int in){
	char chr[20] = {0};
	sprintf(chr, "%d", in);
	std::string re(chr);
	return re;
}
static std::string toStr(uint64_t in){
	char chr[30] = {0};
	sprintf(chr, "%lu", in);
	std::string re(chr);
	return re;
}
static std::string to3dStr(int in){
	char chr[20] = {0};
	sprintf(chr, "%03d", in);
	std::string re(chr);
	return re;
}
static std::string to6dStr(int in){
	char chr[20] = {0};
	sprintf(chr, "%06d", in);
	std::string re(chr);
	return re;
}
static int toInt(const std::string &in){
	int re = 0;
	sscanf(in.c_str(), "%d", &re);
	return re;
}
static uint64_t toIntL(const std::string &in){
	uint64_t re = 0;
	sscanf(in.c_str(), "%lu", &re);
	return re;
}
static std::string toStr(float in){
	char chr[20] = {0};
	sprintf(chr, "%f", in);
	std::string re(chr);
	return re;
}
static float toFloat(const std::string &in){
	float re = 0;
	sscanf(in.c_str(), "%f", &re);
	return re;
}
static void splitStr(const std::string& inputStr, const std::string &key, std::vector<std::string>& outStrVec) {  
	if(inputStr == ""){
		return;
	}
	int pos = inputStr.find(key);
	int oldpos = 0;
	if(pos > 0){
		std::string tmp = inputStr.substr(0, pos);
		outStrVec.push_back(tmp);
	}
	while(1){
		if(pos < 0){
			break;
		}
		oldpos = pos;
		int newpos = inputStr.find(key, pos + key.length());
		std::string tmp = inputStr.substr(pos + key.length(), newpos - pos - key.length());
		outStrVec.push_back(tmp);
		pos = newpos;
	}
	int tmplen = 0;
	if(outStrVec.size() > 0){
		tmplen = outStrVec.at(outStrVec.size() - 1).length();
	}
	if(oldpos+tmplen < (int)inputStr.length()-1){
		std::string tmp = inputStr.substr(oldpos + key.length());
		outStrVec.push_back(tmp);
	}
} 

static std::string rplStr(std::string &str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
    return str;
}

static std::string trim(std::string &s){  
    if (s.empty()){  
        return s;  
    }  
  
    s.erase(0,s.find_first_not_of(" "));  
    s.erase(s.find_last_not_of(" ") + 1);  
    return s;  
}  

static std::string getFileName(const std::string &path){
	std::string re;
	if(path.empty()){
		return re;
	}
	int pos1 = path.rfind("/");
	int pos2 = path.rfind(".");
	if(pos1<0 ||
		pos2<2 ||
		pos1>=pos2){
		return re;
	}
	re = path.substr(pos1+1, pos2-pos1-1);
	return re;
}
#endif
