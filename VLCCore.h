#ifndef _VLCCOREH_
#define _VLCCOREH_

#include <memory>
#include <string>
#include <vector>
#include <iterator>
#include <boost/shared_ptr.hpp>

extern "C" {
#include <libavutil/opt.h>
#include <libavdevice/avdevice.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
}


typedef void (*CBFun)(cv::Mat &frame, int num);

static int ICB(void *ctx);
class VLCCore{
public:
        VLCCore(const std::string &url, CBFun fun){
                num_ = 0;
                url_ = url;
                fun_ = fun;
		lastReadTm_ = 0;
		isRead_ = false;
		startTime_ = 0;
		sleepus_ = 0;
        }
        CBFun fun_;
	uint64_t lastReadTm_;
	bool isRead_;
private:
	uint64_t sleepus_;
	AVFormatContext *inputCtx_ = nullptr;
	AVCodecContext *decodeCtx_ = nullptr;
	AVFrame *bgrFrame_ = nullptr;
	uint8_t *bgrFrameBuf_ = nullptr;
	AVFrame *avframe_ = nullptr;
	SwsContext *swsCtx_ = nullptr;

	uint64_t num_;
        std::string url_;
	uint64_t startTime_;
	AVStream *st_ = NULL;
public:
        void AddFrame(Mat &frame){
		static int i_ = 0;
		fun_(frame, i_++);
        }
public:
	~VLCCore() {
	}
	static void Init(){
		av_register_all();
		avformat_network_init();
	}
        void Start(){
		DoCore();
        }
private: 
	void Beg(){
                lastReadTm_ = gtm();
                isRead_ = true;
	}
	void End(){
		isRead_ = false;
	}
	void DoCore(){
		LOG(INFO) << "VLCCORE:DoCore:url" << url_.c_str();

		while (1) {//begin while
			int stream_index = 0;
			bool rerun = false;
			if(!Open(url_, stream_index, &st_, rerun)){
				break;
			}

			if(rerun){
				LOG(ERROR) << "open stream error, reopen it......";
				continue;
			}
			int video_width = st_->codec->width;
			int video_height = st_->codec->height;


			bgrFrame_ = av_frame_alloc();
			if (!bgrFrame_) {
				LOG(ERROR) << ("VLCCORE: JJJ\n");
				Release();
				continue;
			}
			int picSize = avpicture_get_size(AV_PIX_FMT_BGR24, video_width, video_height);
			if(picSize <= 0){
				LOG(ERROR) << "picSize==0, width:" << video_width << ", height:" << video_height;
				Release();
				continue;
			}
			/*std::vector<uint8_t> bgrFrameBuf(picSize);
			int ret = avpicture_fill(reinterpret_cast<AVPicture*>(bgrFrame_), bgrFrameBuf.data(), AV_PIX_FMT_BGR24, video_width, video_height);
			if (ret != 0) {
				LOG(ERROR) << "VLCCORE: " << "avpicture_fill error ret = " << ret << ",width:" << video_width << ",height:" << video_height;
				//????????????????? xyz 2017.09.20 add
				//Release();
				//continue;
			}*/
			bgrFrameBuf_ = (uint8_t*)av_malloc(picSize*sizeof(uint8_t));
			int ret = avpicture_fill((AVPicture*)bgrFrame_, bgrFrameBuf_, AV_PIX_FMT_BGR24, video_width, video_height);
			if (ret != 0) {
				char disp[1024];
				av_strerror(ret, disp, 1024);
				LOG(ERROR) << "VLCCORE: " << "avpicture_fill error ret = " << ret << ",width:" << video_width << ",height:" << video_height << ", disp:" << disp;

				//????????????????? xyz 2017.09.20 add
				//Release();
				//continue;
			}
			
			avframe_ = av_frame_alloc();
			if (!avframe_) {
				LOG(ERROR) << ("VLCCORE: KKK\n");
				Release();
				continue;
			}


			startTime_ = av_gettime();
			AVRational timeBase = st_->time_base;
			AVRational baseq = {1, AV_TIME_BASE};
			AVPacket packet;
			//av_init_packet(&packet);

			int retryCount = 0;
			while(1){
				if(!ReadCore(packet, 
					stream_index, 
					video_width, 
					video_height, 
					timeBase, 
					baseq)){//,
					//bgrFrameBuf)){
					retryCount++;
					if(retryCount > 2){
						LOG(ERROR) << "retryCount>2, break readcore";
						break;
					}
					continue;
				}
				retryCount = 0;
			}
			Release();
			//break;
			LOG(INFO) << "reread file again!(cur num_:" << num_ << ")";
		}// end while

		LOG(INFO) << ("set stop vlccore finish\n");
	}
	bool Open(const std::string &infile, int &stream_index, AVStream **video_stream, bool &rerun){


		int ret;
		inputCtx_ = avformat_alloc_context();
		if (!inputCtx_) {
			LOG(ERROR) << ("VLCCORE: AAA\n");
			Release();
			rerun = true;
			return true;
		}
		{
			inputCtx_->interrupt_callback.callback = ICB;
			inputCtx_->interrupt_callback.opaque = (void*)this;
		}

		AVDictionary *opts = nullptr;
		ret = av_dict_set(&opts, "rtsp_transport", "tcp", 0);
		//ret = av_dict_set(&opts, "fflags", "nobuffer", 0);
		if (ret != 0) {
			LOG(ERROR) << "VLCCORE: BBB av_dict_set error ret = "  << ret;
			Release();
			rerun = true;
			return true;
		}
		Beg();
		ret = avformat_open_input(&inputCtx_, infile.c_str(), NULL, &opts);

		av_dict_free(&opts);// xyz: 20170712 add


		End();
		if (ret != 0) {
			LOG(ERROR) << "VLCCORE: CCC("
				<< infile.c_str() << ") av_format_open_input error ret = " << ret;
			Release();
			rerun = true;
			return true;
		}
		Beg();
		ret = avformat_find_stream_info(inputCtx_, NULL);
		End();
		if (ret != 0) {
			LOG(ERROR) << "VLCCORE: DDD avformat_find_stream_info error ret = " << ret;
			Release();
			rerun = true;
			return true;
		}

		Beg();
		ret = av_find_best_stream(inputCtx_, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
		End();
		if (ret != 0) {
			LOG(ERROR) << "VLCCORE: EEE av_find_best_stream error ret = " << ret;
			Release();
			rerun = true;
			return true;
		}

		stream_index = ret;
		*video_stream = inputCtx_->streams[ret];
		{
			AVRational fr = (*video_stream)->r_frame_rate;
			if(0 != fr.num){
				sleepus_ = fr.den*1000*1000/fr.num;
			}
		}
#ifdef FFMPEG33
		AVCodec *codec = avcodec_find_decoder((*video_stream)->codecpar->codec_id);

		decodeCtx_ = avcodec_alloc_context3(codec);
		if (!decodeCtx_) {
			LOG(ERROR) << ("VLCCORE: GGG\n");
			Release();
			rerun = true;
			return true;
		}
		ret = avcodec_parameters_to_context(decodeCtx_, (*video_stream)->codecpar);
		if (ret < 0) {
			LOG(ERROR) << ("VLCCORE: HHH\n");
			Release();
			rerun = true;
			return true;
		}
#else
		AVCodec *codec = avcodec_find_decoder((*video_stream)->codec->codec_id);
		if (!codec) {
			LOG(ERROR) << ("VLCCORE: FFF\n");
			Release();
			rerun = true;
			return true;
		}
		decodeCtx_ = (*video_stream)->codec;
#endif
		ret = avcodec_open2(decodeCtx_, codec, nullptr);
		if (ret != 0) {
			LOG(ERROR) << "VLCCORE: III avcodec_open2 error ret = " << ret;
			Release();
			rerun = true;
			return true;
		}
		return true;
	}

	bool ReadCore(AVPacket &packet, 
			const int &stream_index, 
			const int &video_width, 
			const int &video_height,
			const AVRational &timeBase, 
			const AVRational &baseq){//, 
			//std::vector<uint8_t> &bgrFrameBuf){ 

		Beg();
		int ret = av_read_frame(inputCtx_, &packet);
		End();
		if (ret != 0) {
			char disp[1024];
			av_strerror(ret, disp, 1024);
			LOG(ERROR) << "av_read_frame error ret = " << ret << ",disp:" << disp;
			av_packet_unref(&packet);
			av_free_packet(&packet);
			//if (!isFile_) {// camera continue
			//	continue;
			//}
			return false; // all break
		}

		if (packet.stream_index != stream_index) { // packet is not video
			av_packet_unref(&packet);
			av_free_packet(&packet);
			return true;
		}
		int got_pic = 0;
		//uint64_t tm1 = gtm();
		//{
		//	avcodec_free_frame(&avframe_);
		//	avframe_ = avcodec_alloc_frame();
		//}
		ret = avcodec_decode_video2(decodeCtx_, avframe_, &got_pic, &packet);
		//uint64_t tm2 = gtm();
		if (!got_pic) {
			LOG(ERROR) << "avcodec_decode_video2 error ret = " <<ret;
			av_packet_unref(&packet);
			av_free_packet(&packet);
			return true;
		}

		if (!swsCtx_) {
			std::string pfn = av_get_pix_fmt_name((AVPixelFormat)avframe_->format);
			LOG(INFO) << "avframe->format:"
				<< pfn.c_str() << "video_width:" << video_width << "video_height:" << video_height;
			swsCtx_ = sws_getContext(video_width, 
						video_height,
						static_cast<enum AVPixelFormat>(avframe_->format),
						video_width, 
						video_height, 
						AV_PIX_FMT_BGR24,
						SWS_FAST_BILINEAR,//SWS_BICUBIC, 
						NULL, 
						NULL, 
						NULL);
					LOG(INFO) << "SWD_CTX END";
		}
		if (!swsCtx_) {
			LOG(ERROR) << "sws_ctx is null";
			av_packet_unref(&packet);
			av_free_packet(&packet);
			return false;
		}
		//uint64_t tm3 = gtm();
		if (sws_scale(swsCtx_, avframe_->data, avframe_->linesize, 0, video_height,
			bgrFrame_->data, bgrFrame_->linesize) <= 0) {
			LOG(ERROR) << "sws_scale error";
			av_packet_unref(&packet);
			av_free_packet(&packet);
			return false;
		}
		{
			av_frame_unref(avframe_);
			//avcodec_get_frame_defaults(avframe_);
		}
		//uint64_t tm4 = gtm();
		//cv::Mat image(video_height, video_width, CV_8UC3, bgrFrameBuf.data(), bgrFrame_->linesize[0]);
		cv::Mat image(video_height, video_width, CV_8UC3, bgrFrameBuf_, bgrFrame_->linesize[0]);
		//uint64_t tm5 = gtm();
		uint64_t pkt = av_rescale_q(packet.pts, timeBase, baseq);
		this->AddFrame(image);
		av_packet_unref(&packet);
		av_free_packet(&packet);
		//LOG(INFO) << "read one frame sucess!!!";
		return true;	
	}
	void Release() {
		if (swsCtx_) {
			sws_freeContext(swsCtx_);
			swsCtx_ = nullptr;
		}
		if (avframe_) {
			av_frame_free(&avframe_);
			avframe_ = nullptr;
		}
		if (bgrFrame_) {// 2017.06.16
			av_frame_free(&bgrFrame_);
			bgrFrame_ = nullptr;
		}
#ifdef FFMPEG33
		if (decodeCtx_) {
			avcodec_free_context(&decodeCtx_);
			decodeCtx_ = nullptr;
		}
#else// 2017.10.23 the leak
		avcodec_close(st_->codec);		
#endif
		if (inputCtx_) {
			avformat_close_input(&inputCtx_);
			avformat_free_context(inputCtx_);
			inputCtx_ = nullptr;
		}
		if(bgrFrameBuf_){
			av_free(bgrFrameBuf_);
			bgrFrameBuf_ = nullptr;
		}

	}
};

static int ICB(void *ctx){
	return 0;

	VLCCore *vlccore = (VLCCore*)ctx;
	if(!vlccore->isRead_){
		return 0;
	}
	// do only for read frame
	uint64_t tm = gtm();
	if(tm-vlccore->lastReadTm_ > 15000000){
		LOG(ERROR) << ("vlccore read timeout\n");
		return 1;
	}
	return 0;
}

#endif
