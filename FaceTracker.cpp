#include "FaceTracker.h"


#include "StrCommon.h"
#include "fdsst/fdssttracker.hpp"
#include "fdsst/fhog.h"
#include <boost/thread/mutex.hpp>

#ifdef USETBB
#include <tbb.h>
#endif

#include <Eigen>
#include<StdVector>
#define GLOG_NO_ABBREVIATED_SEVERITIES
#include <glog/logging.h>


//#define UHOG

#if 1
typedef Eigen::Matrix<float, 1, 4, Eigen::RowMajor> DSBOX;
typedef Eigen::Matrix<float, -1, 4, Eigen::RowMajor> DSBOXS;
typedef Eigen::Matrix<float, 1, 128, Eigen::RowMajor> FEATURE;
typedef Eigen::Matrix<float, -1, 128, Eigen::RowMajor> FEATURESS;
typedef std::vector<int> IDS;
typedef Eigen::Matrix<float, 1, 2, Eigen::RowMajor> PT2;
typedef Eigen::Matrix<float, -1, -1, Eigen::RowMajor> DYNAMICM;
typedef Eigen::Matrix<float, 1, 8, Eigen::RowMajor> MEAN;
typedef Eigen::Matrix<float, 8, 8, Eigen::RowMajor> VAR;
typedef Eigen::Matrix<float, 1, 4, Eigen::RowMajor> NMEAN;
typedef Eigen::Matrix<float, 4, 4, Eigen::RowMajor> NVAR;

#else
typedef Eigen::Matrix<float, 1, 4> DSBOX;
typedef Eigen::Matrix<float, -1, 4> DSBOXS;
typedef Eigen::Matrix<float, 1, 128> FEATURE;
typedef Eigen::Matrix<float, -1, 128> FEATURESS;
typedef std::vector<int> IDS;
typedef Eigen::Matrix<float, 1, 2> PT2;
typedef Eigen::Matrix<float, -1, -1> DYNAMICM;
typedef Eigen::Matrix<float, 1, 8> MEAN;
typedef Eigen::Matrix<float, 8, 8> VAR;
typedef Eigen::Matrix<float, 1, 4> NMEAN;
typedef Eigen::Matrix<float, 4, 4> NVAR;

#endif

typedef std::vector<FEATURE, Eigen::aligned_allocator<FEATURE>> FEATUREVEC;


void ExtractFeatureHog(const cv::Mat &in, 
	const std::vector<cv::Rect> &rcsin,
	FEATUREVEC &fts){
	cv::Mat frame;
	cvtColor(in, frame, cv::COLOR_RGB2GRAY);
	for(int i = 0; i < (int)rcsin.size(); i++){
		Mat nnn = frame(rcsin[i]);
		resize(nnn, nnn, Size(32, 32));
		int len = 0;
		float *hog = HOGXYZ(nnn, len);
		if(hog==NULL || len!=128){
			LOG(ERROR) << "hog(" << (hog==NULL) << ") is null or len(" << len << ")!=128,exit!";
			exit(0);
		}
		FEATURE ft;
		for(int j = 0; j < len; j++){
			ft(j) = hog[j];
		}
		delete []hog;
		fts.push_back(ft);
	}
}

FaceTracker::FaceTracker(){
}

FaceTracker::~FaceTracker(){
}
bool FaceTracker::Init(){
	Eigen::initParallel();

	return true;
}


struct RRSN{
	void Push(int pos, const cv::Rect &rc){
#ifndef USETBB
		boost::mutex::scoped_lock lock(mutex_);
#endif
		rcs_.push_back(std::make_pair(pos, rc));
	}
	void Get(std::vector<std::pair<int, cv::Rect>> &rcs){
#ifndef USETBB
		rcs = rcs_;
#else
		for(int i = 0; i < (int)rcs_.size(); i++){
			std::pair<int, cv::Rect> tmp = rcs_[i];
			rcs.push_back(tmp);
		}
#endif
	}
private:
#ifndef USETBB
	std::vector<cv::Rect> rcs_;
	boost::mutex mutex_;
#else
	tbb::concurrent_vector<std::pair<int, cv::Rect>> rcs_;
#endif
};
typedef boost::shared_ptr<RRSN> RRS;


struct FFSN{
public:
	void Push(int id, const FDSSTTrackerP &ff){
#ifndef USETBB
		boost::mutex::scoped_lock lock(mutex_);
#endif
		ffs_.push_back(std::make_pair(id, ff));
	}
	void Get(std::vector<std::pair<int, FDSSTTrackerP> > &ffs){
#ifndef USETBB
		ffs = ffs_;
#else
		for(int i = 0; i < (int)ffs_.size(); i++){
			ffs.push_back(ffs_[i]);
		}
#endif
	}
private:
#ifndef USETBB
	std::vector<std::pair<int, FDSSTTrackerP > > ffs_;
	boost::mutex mutex_;
#else
	tbb::concurrent_vector<std::pair<int, FDSSTTrackerP> > ffs_;
#endif
};
typedef boost::shared_ptr<FFSN> FFS;

// for framebuffer
void FaceTracker::UpdateFDSST(const Mat &frame, std::vector<cv::Rect> &rcs){
	std::map<int, FDSSTTrackerP>::iterator it;
	std::vector<int> lostIds;
	RRS rrs(new RRSN());
	std::vector<std::pair<int, FDSSTTrackerP>> ffs;
	for(it = fdssts_.begin(); it != fdssts_.end(); ++it){
		std::pair<int, FDSSTTrackerP> pa = *it;//fdsst = it->second;
		ffs.push_back(pa);
	}
	#ifdef USETBB
	using namespace tbb;
	parallel_for( blocked_range<int>(0, (int)ffs.size()),
		[=](const blocked_range<int> &r){
		for(int i = r.begin(); i != r.end(); i++){
			cv::Rect rc = ffs[i].second->update(frame);
			rrs->Push(ffs[i].first, rc);
		}
	});
	#else
	omp_set_num_threads(4);
	#pragma omp parallel for
	for(int i = 0; i < (int)ffs.size(); i++){
		cv::Rect rc = ffs[i]->update(frame);
		rrs->Push(rc);
	}
	#endif
	//
	std::vector<std::pair<int, cv::Rect>> rrcs;
	rrs->Get(rrcs);
	for(int i = 0; i < (int)rrcs.size(); i++){
		std::pair<int, cv::Rect> pa = rrcs[i];
		cv::Rect rc = pa.second;//rrcs[i];
		int ww = frame.cols;
		int hh = frame.rows;
		int min = 8;
                /*if(rc.x<0 || rc.y<0 ||
                        (rc.x+rc.width)>ww ||
                        (rc.y+rc.height)>hh ||
                        rc.width<=min || rc.height<=min){
			lostIds.push_back(pa.first);//it->first);
			continue;
		}*/
		rcs.push_back(ToOriRect(rc));
	}
	// remove
	/*for(int id:lostIds){
		LOG(ERROR) << "erase:" << id;
		fdssts_.erase(id);
	}*/
}
std::map<int, DSResult> FaceTracker::UpdateAndGet(const cv::Mat &frame, 
	const std::vector<cv::Rect> &rcsin, 
	int num,
	std::vector<cv::Rect> &outRcs,
	const std::vector<int> &oriPos,
	NewAndDelete *nadOut/*=NULL*/){
	std::vector<cv::Rect> rcs;// = rcsin;
        
	//{	
	Mat ffMat;
        cvtColor(frame, ffMat, cv::COLOR_RGB2GRAY);
	resize(ffMat, ffMat, Size(ffMat.cols*scale_, ffMat.rows*scale_));
	LOG(INFO) << "FaceTracker::UpdateAndGet1\n";
	//}
#ifdef UFDSST
	if(!rcsin.empty()){
		fdssts_.clear();
		rcs = rcsin;
		std::vector<std::pair<int, cv::Rect>> idrcs;
		int i = 0;
		for(cv::Rect rc:rcsin){
			std::pair<int, cv::Rect> pa;
			pa.first = i++;
			pa.second = rc;
			idrcs.push_back(pa);
		}		

		FFS ffs(new FFSN());
		using namespace tbb;
		parallel_for( blocked_range<int>(0, idrcs.size()),
			[=](const blocked_range<int> &r){
			for(int i = r.begin(); i != r.end(); i++){
			std::pair<int, cv::Rect> pa = idrcs[i];
			int id = pa.first;
			cv::Rect rc = pa.second;
			LOG(INFO) << "id:" << id << ", rc:(" << rc.x << ", "
				<< rc.y << ", " << rc.width << ", " 
				<< rc.height; 

			FDSSTTrackerP fdsst(new FDSSTTracker());
			fdsst->init(ToScaleRect(rc), ffMat);
			ffs->Push(id, fdsst);
			}
		});
		LOG(INFO) << "FaceTracker::UpdateAndGet4\n";
		std::vector<std::pair<int, FDSSTTrackerP> > pps;
		ffs->Get(pps);
		for(int i = 0; i < (int)pps.size(); i++){
			std::pair<int, FDSSTTrackerP> pa = pps[i];
			fdssts_.insert(pa);
		}

	}	
	else{
		UpdateFDSST(ffMat, rcs);
	}
#endif
	LOG(ERROR) << "rcs.size():" << rcs.size();
	std::map<int, DSResult> map;
	int id = 0;
	for(cv::Rect rc:rcs){
		DSResult dr;
		dr.rc_ = rc;
		map.insert(std::make_pair(id++, dr));
	}
	return map;
}





