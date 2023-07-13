# snpe-docker
SNPE-DOCKER make the complete env setup instantly ready for snpe.<br>
* It offers set-up for all major machine learning frameworks like tensorflow, caffe-ssd, torch, onnx and tflite.
* All set-up for "Qualcomm Neural Processing SDK for AI" are done.
* ADB support is present within the docker.
* Many other python dependencies are installed in the docker.
* This docker file is verified for version - 2.12.* of _Snapdragon Neural Processing SDK for AI_

### Docker Installation
* --update-later-- https://docs.docker.com/engine/install/ubuntu/

### Directory structure

Please maintain below directory structure, while building, and loading docker container. 

1. Download SNPE SDK
2. Rename SNPE SDK to just 'snpe' (small case), and place in the docker directory. 
3. Dockerfile - to be cloned from open-source repository.
4. snpe-docker.zip - will get generated, once below steps are complete. 

├── snpe-docker<br>
│   │── snpe  (unziped snpe sdk folder) <br>
│   │── Dockerfile<br>
│   │── snpe-docker.zip<br>

### Dockerfile
Contains script for installation of all of the packges required in docker image.

### Build Docker
-t flag is used to give tag names to the docker
```python
docker build -t snpe-docker .
```
  
### Save Docker 
```python
docker save -o snpe-docker.tar snpe-docker 
```
<b>NOTE:</b> Before loading image, please make sure to delete previous container image. <br>
Run below commands to delete an existing container with same tag name.
* Run ```docker images``` and copy the IMAGE_ID you want to delete.
* Run ```docker rmi -f <paste IMAGE_ID here>```

### Load Docker 
```python
docker load < snpe-docker.tar
```

### Run Docker container
-it : is for interactive session <br>
--rm : once it's done running, erase everything related to it <br>
-v : to mount volume <br>
--net : flag set as "host" to detect connected device from host using adb <br>

```python
docker run -it --rm --privileged -v $(pwd):/workspace --net host snpe-docker
```
