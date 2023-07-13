FROM ubuntu:20.04
FROM python:3.8

ENV OPENCV_VERSION="3.4.16"
ARG DEBIAN_FRONTEND=noninteractive

RUN python -m pip install --upgrade pip

RUN set -ex \
&& apt-get -qq update \
\
&& echo "Install requirements using pip" \
\
&& pip install \
numpy==1.20.0 \
scipy==1.3.2 \
mako==1.1.0 \
matplotlib==3.3.4 \
protobuf==3.19.6 \
onnx==1.11.0 \
tensorflow==2.10.1 \
torch==1.8.1 \
tflite==2.3.0 \
opencv-python==4.5.2.52 \
absl-py==1.0.0 \
decorator==5.1.1 \
invoke==2.0.0 \
joblib==1.0.1 \
lxml==4.6.2 \
packaging==21.0 \
pandas==0.25.3 \
pathlib2==2.3.6 \
pillow==6.2.1 \
six==1.16.0 \
tabulate==0.8.5 \
notebook \
onnxsim \
transformers \
\
&& apt-get update \
&& apt-get install -qq -y gcc g++ wget zip git android-tools-adb android-tools-fastboot libc++1 libgl1 \
&& apt-get -qq -y --no-install-recommends install \
libprotobuf-dev protobuf-compiler \
libleveldb-dev libsnappy-dev libhdf5-serial-dev \
libboost-all-dev \
libgflags-dev libgoogle-glog-dev liblmdb-dev nano
# Prepare SNPE and other var
ENV SNPE_ROOT=/workspace/snpe
ENV PYTHONPATH="${SNPE_ROOT}/lib/python:${PYTHONPATH}" \
LD_LIBRARY_PATH="/usr/local/lib:/usr/lib:${SNPE_ROOT}/lib/x86_64-linux-clang:${LD_LIBRARY_PATH}" \
PATH="${SNPE_ROOT}/bin/x86_64-linux-clang:${PATH}" \
TENSORFLOW_ROOT="/usr/local/lib/python3.8/site-packages/tensorflow" \
SNPE_UDO_ROOT="${SNPE_ROOT}/share/SnpeUdo"
WORKDIR /workspace
CMD ["/bin/bash"]
