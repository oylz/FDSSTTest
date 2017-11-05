#!/bin/bash

function getbazel(){
	LINE=`readlink -f /home/$USER/code1/tensorflow-1.4.0-rc0/bazel-bin/`

	POS1="_bazel_$USER/"
	STR=${LINE##*$POS1}

	BAZEL=${STR:0:32}

	echo $BAZEL
}



BAZEL=`getbazel`
APPN="FDSSTTest"

#add export OPENCV_INCLUDEPATH=/home/$USER/code/test/pp/opencvlib/include to .bashrc
#add export OPENCV_LIBPATH=/home/$USER/code/test/pp/opencvlib/lib to .bashrc
#add export CAFFEROOT="/home/xyz/code1/py-faster-rcnn-master/caffe-fast-rcnn" to .bashrc
#add export TBBROOT=.. to .bashrc


IINCLUDE="-I$OPENCV_INCLUDEPATH -I/usr/local/include -I/home/$USER/.cache/bazel/_bazel_$USER/$BAZEL/external/eigen_archive/Eigen -I$TBBROOT/include -I$TBBROOT/include/tbb"

TBBLIBWHOLEPATH=`find $TBBROOT  -name "libtbb.so"`
TBBLIBPATH=${TBBLIBWHOLEPATH/\/libtbb.so/}

LLIBPATH="-L$OPENCV_LIBPATH -L/usr/local/lib -L/home/$USER/code1/$APPN/deepsort/FeatureGetter -L$TBBLIBPATH "

rm $APPN -rf



function FF(){
	# if has FFMPEG_PATH
	if [  "A$FFMPEG_PATH" != "A"   ]
	then
		IINCLUDE="$IINCLUDE -I$FFMPEG_PATH/include"
	fi

	# ========lib path================================================
	LLIBPATH="$LLIBPATH -L$OPENCV_LIBPATH -L$FACETRACKER_PATH"
	# if has FFMPEG_PATH
	if [  "A$FFMPEG_PATH" != "A"   ]
	then
		LLIBPATH="$LLIBPATH -L$FFMPEG_PATH/lib"
	fi
	LLIBS="$LLIBS -lavcodec"
	LLIBS="$LLIBS -lavdevice"
	LLIBS="$LLIBS -lavfilter"
	LLIBS="$LLIBS -lavformat"
	LLIBS="$LLIBS -lavutil"
	LLIBS="$LLIBS -lswscale"

	#
	FFMPEG33=`ls  $FFMPEG_PATH/lib | grep avcodec  | grep so.57 | wc -l`
	if [ $FFMPEG33 -gt 0 ]
	then
		DDEFINES="$DDEFINES -DFFMPEG33"
	fi

}
function BCore(){
	LLIBS="$LLIBS -ltcmalloc -lDetector"

        OOS=`cat /proc/version | grep "Red Hat" | wc -l`
        if [ $OOS -gt 0 ]
        then
                DDEFINES="$DDEFINES -DCENTOS"
        fi
	IINCLUDE="$IINCLUDE -I$OPENCV_INCLUDEPATH/opencv -I$CAFFEROOT/include"
	IINCLUDE="$IINCLUDE -I/usr/local/cuda/include"
	IINCLUDE="$IINCLUDE -I$CAFFEROOT/build/src -I/usr/include/python2.7"

	CV24=`ls $OPENCV_LIBPATH | grep opencv | grep so.2.4 | wc -l`
	if [ $CV24 -gt 0 ]
	then
		DDEFINES="$DDEFINES -DOPENCV24"
		LLIBS="$LLIBS -l:libopencv_core.so.2.4 -l:libopencv_imgproc.so.2.4  -l:libopencv_highgui.so.2.4  -lboost_system -lglog"
	else
		LLIBS="$LLIBS -lDeepirIO -lDeepirAlgorithm -lDeepirFaceAlgorithm -lcaffe -l:libopencv_imgcodecs.so.3.2 -l:libopencv_videoio.so.3.2 -lpython2.7 -lboost_python"
		LLIBS="$LLIBS -l:libopencv_core.so.3.2 -l:libopencv_imgproc.so.3.2  -l:libopencv_highgui.so.3.2  -lboost_system -lglog"
	fi


	LLIBPATH="$LLIBPATH -L$OPENCV_LIBPATH -L$CAFFEROOT/distribute/lib -L/home/xyz/code1/FaceTracker/detectalign"

	FF

	g++ --std=c++14 -fopenmp $DDEFINES -o $APPN $IINCLUDE $LLIBPATH   FaceTracker.cpp fdsst/fdssttracker.cpp fdsst/fhog.cpp Main.cpp $LLIBS
}



DDEFINES="$DDEFINES -DWITHDETECT -DUFDSST"
#DDEFINES="$DDEFINES -DUFDSST"

function BHOG(){
	DDEFINES="$DDEFINES -DUHOG"
	BCore
}

function BDL(){
	DDEFINES="$DDEFINES -DUDL"
	LLIBS="$LLIBS -lFeatureGetter"
	LLIBPATH="$LLIBPATH -L./deepsort/FeatureGetter"
	BCore
}

DDEFINES="$DDEFINES -DUSETBB"
LLIBS="$LLIBS -ltbb"
BHOG




