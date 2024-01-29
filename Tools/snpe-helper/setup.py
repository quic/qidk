# -*- mode: python -*-
# =============================================================================
#  @@-COPYRIGHT-START-@@
#
#  Copyright (c) 2023 of Qualcomm Innovation Center, Inc. All rights reserved.
#  SPDX-License-Identifier: BSD-3-Clause
#
#  @@-COPYRIGHT-END-@@
# =============================================================================

import os
from setuptools import setup
import subprocess
from prebuilt_binaries import prebuilt_binary, PrebuiltExtension

def build_cmake():
    os.chdir("snpehelper")
    if not os.path.exists("build"):
        os.mkdir("build")
    os.chdir("build")

    vs_version = " \"Visual Studio 17 2022\""

    subprocess.run("cmake .. -G " + vs_version + " -A ARM64EC " + " -DCHIPSET=SC8380")
    subprocess.run("cmake --build ./ --config Release ")
    os.chdir("../../")

build_cmake()

with open("README.md", "r") as fh:
    long_description = fh.read()

setup(
    name='snpehelper',
    version='0.1.0',
    cmdclass={
        'build_ext': prebuilt_binary,
    },
    ext_modules=[PrebuiltExtension("snpehelper/build/Release/snpehelper.pyd")],
    description='Python API wrapper over SNPE APIs for exection with CPP backend',
    long_description=long_description,
    long_description_content_type="text/markdown",
    url='https://github.com/quic/qidk',
    author='Sumith Kumar Budha',
    author_email='quic_sbudha@quicinc.com',
    license='BSD 3-clause',
    python_requires='>=3.8',
    install_requires=['matplotlib>=3.7.3',
                      'numpy>=1.24.3',
                      'opencv-python>=4.8.0.76',
                      'Pillow>=10.0.1',
                      'pybind11>=2.11.1',
                      'tqdm>=4.66.1',
                      'torch>=2.1.0',
                      'torchvision>=0.16.0',
                      'diffusers==0.22.3',
                      'accelerate>=0.24.1'
                      ],
    classifiers=[
        'Development Status :: 1 - Planning',
        'Intended Audience :: Qualcomm AI Developer Ecosystem',
        'License :: OSI Approved :: BSD License',
        'Operating System :: Windows On Snapdragon"',
        'Programming Language :: Python :: 3.8',
    ],
)
