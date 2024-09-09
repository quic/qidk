Support:
========

This docker file can be used on SNPE and QNN Versions: 2.25 and above

Prerequisites:
==============
- (Required) Downloaded QNN SDK (prefered unzipped)
- (Optional) Android NDK (will be downloaded automatically)

Steps to create the docker container:
=====================================
- Make sure both the dockerfile and setup_env.sh script are present at the same location:

		Sample: "docker build -t <image_name> ."
		To run: "docker build -t snpe_qnn ."
	
- Create a container from the image:
	
		Sample: docker run --net host -dit --name <container_name> --mount type=bind,source="/local/",target="/local/" <image_name>:latest

		To run: docker run --net host -dit --name snpe_qnn_container --mount type=bind,source="/local/",target="/local/" snpe_qnn:latest

- Open the container:
	
        Sample: docker exec -it <container_name> /bin/bash
		To run: docker exec -it snpe_qnn_container /bin/bash
	* While passing the SDK path please give complete path ending with a "/". Example: "/local/mnt/workspace/sdk/2.25/"

Note : You can install any pip package in docker as per your requirement.

Steps to remove the docker container (if needed):
=================================================
- <b>These Steps are not at all mandatory, use these only to get rid of docker not in use anymore.</b>

- Identify the docker container you want to stop.

		docker ps -a
- Once you have the CONTAINER ID from above command. Run below command to stop a running docker container.

		docker stop <CONTAINER ID>
- Deleting the docker container.

		docker rm <CONTAINER ID>

- Finally, delete the docker image as well. Identify IMAGE ID by running:

		docker images
- Once you have IMAGE ID from above command, you can delete docker image:

		docker rmi -f <IMAGE ID>