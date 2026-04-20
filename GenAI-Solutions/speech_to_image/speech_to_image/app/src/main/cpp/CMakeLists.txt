cmake_minimum_required(VERSION 3.10)

project(qnn-models-native)

add_subdirectory(speech_to_image)
add_subdirectory(prepare)

add_custom_target(qnn-models-native ALL
        DEPENDS speech_to_image-native prepare-native
)