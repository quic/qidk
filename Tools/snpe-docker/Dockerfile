FROM ubuntu:18.04
FROM python:3.6

ENV OPENCV_VERSION="3.4.16"
ARG DEBIAN_FRONTEND=noninteractive

COPY Makefile.config /tmp
RUN python -m pip install pip==21.3.1

RUN set -ex \
	&& apt-get -qq update \
	\
    && apt-get install -qq -y \
	build-essential cmake git pkg-config libgtk-3-dev \
    libavcodec-dev libavformat-dev libswscale-dev libv4l-dev \
    libxvidcore-dev libx264-dev libjpeg-dev libpng-dev libtiff-dev \
    gfortran openexr libatlas-base-dev python3-dev python3-numpy \
    libtbb2 libtbb-dev libdc1394-22-dev \
	&& cd /opt \
	&& mkdir /opt/opencv_build && cd /opt/opencv_build \
	&& wget -q --no-check-certificate https://github.com/opencv/opencv/archive/refs/tags/${OPENCV_VERSION}.zip -O opencv.zip \
    && unzip opencv.zip \
	&& rm -rf opencv.zip \
	&& mv opencv-${OPENCV_VERSION} opencv\
	&& wget -q --no-check-certificate https://github.com/opencv/opencv_contrib/archive/refs/tags/${OPENCV_VERSION}.zip -O opencv_contrib.zip \
	&& unzip opencv_contrib.zip \
	&& rm -rf opencv_contrib.zip \
	&& mv opencv_contrib-${OPENCV_VERSION} opencv_contrib \
	&& cd /opt/opencv_build/opencv \
	&& mkdir build && cd build \
	&& cmake -D CMAKE_BUILD_TYPE=RELEASE \
    -D CMAKE_INSTALL_PREFIX=/usr/local/ \
    -D INSTALL_C_EXAMPLES=ON \
    -D INSTALL_PYTHON_EXAMPLES=ON \
    -D OPENCV_GENERATE_PKGCONFIG=ON \
    -D OPENCV_EXTRA_MODULES_PATH=/opt/opencv_build/opencv_contrib/modules \
    -D BUILD_EXAMPLES=ON .. \
	 && make -j8 \
	 && make install \
\
	&& echo "Install requirements using pip" \
    \
    && pip install \
        numpy==1.19.5 \
		sphinx==5.3.0 \
		scipy==1.5.4 \
		mako==1.1.6 \
		matplotlib==3.3.4 \
        scikit-image==0.17.2 \
        protobuf==3.19.6 \
        pyyaml==5.3.1 \
        onnx==1.11.0 \
		onnxsim==0.4.8 \
        onnxruntime==1.10.0 \
        tensorflow==2.6.2 \
        tensorboard==2.6.0 \
		torch==1.10.1 \
		tflite==2.3.0 \
		opencv-python==3.4.8.29 \
		notebook \
		\
	&& echo "dependency for caffe-ssd" \
	&& apt-get update \
	&& apt-get install -qq -y gcc g++ wget zip git android-tools-adb android-tools-fastboot \
	&& apt-get -qq -y --no-install-recommends install \
		    libprotobuf-dev protobuf-compiler \
	 		libleveldb-dev libsnappy-dev libhdf5-serial-dev \
	 		libboost-all-dev \
	 		libgflags-dev libgoogle-glog-dev liblmdb-dev nano \
	\
	&& echo "Build boost" \
	&& cd /opt/ \
	&& wget https://boostorg.jfrog.io/artifactory/main/release/1.77.0/source/boost_1_77_0.tar.gz \
	&& tar xzf boost_1_77_0.tar.gz \
	&& rm -rf boost_1_77_0.tar.gz \
	&& cd boost_1_77_0 \
	&& ./bootstrap.sh \
	&& ./b2 install \
	\
	&& echo "Build caffe" \
	&& cd /opt \
	&& mkdir caffe-build \
	&& cd caffe-build \
	&& git clone https://github.com/weiliu89/caffe.git . \
	&& git checkout ssd \
	&& git reset --hard 289b10ee5cb844278ad235c9fe4280d29f9af311 \
	&& cp /tmp/Makefile.config Makefile.config \
	&& make all \
	&& make pycaffe 
# Prepare SNPE and other var
ENV SNPE_ROOT=/workspace/snpe
ENV PYTHONPATH="${SNPE_ROOT}/lib/python:${PYTHONPATH}" \
	CAFFE_HOME="/opt/caffe-build" \
	LD_LIBRARY_PATH="/usr/local/lib:/usr/lib:${SNPE_ROOT}/lib/x86_64-linux-clang:/opt/caffe-build/build/lib:${LD_LIBRARY_PATH}" 
ENV PYTHONPATH="/opt/caffe-build/python:/opt/caffe-build/build/python:${PYTHONPATH}" \
    PATH="${SNPE_ROOT}/bin/x86_64-linux-clang:${PATH}" \
	TENSORFLOW_ROOT="/usr/local/lib/python3.6/site-packages/tensorflow" \
	SNPE_UDO_ROOT="${SNPE_ROOT}/share/SnpeUdo"
WORKDIR /workspace
CMD ["/bin/bash"]
