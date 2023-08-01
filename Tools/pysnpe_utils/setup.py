from setuptools import setup

with open("README.md", "r") as fh:
    long_description = fh.read()

setup(
    name='pysnpe_utils',
    version='0.1.0',    
    description='Python API wrapper over SNPE Tools and APIs for Auto DLC generation and Auto Accuracy Validation from Python DNN evaluation scripts',
    long_description=long_description,
    long_description_content_type="text/markdown",
    url='https://github.com/quic/qidk',
    author='Shubham Patel (quic-shubpate) and Pradeep Pant (quic-ppant)',
    author_email='quic_shubpate@quicinc.com ;  quic_ppant@quicinc.com',
    license='BSD 3-clause',
    packages=['pysnpe_utils','pysnpe_utils.modelio'],
    python_requires='>=3.6',
    install_requires=['icecream',
                      'numpy>=1.19.0',
                      'torch>=1.8.1',
                      'onnx>=1.11.0',
                      'onnxsim',
                      'onnxruntime==1.10.0',
                      'pytest>=7.0.1',
                      'pure-python-adb>=0.3.0.dev0',
                      'pydot>=1.4.2'
                      ],
    setup_requires=[
        'icecream',
        'numpy>=1.19.0'
    ],
    platform_specific_requires=[
        # Use tensorflow-aarch64 on arm64 architecture with Python 3.7 or later
        ('aarch64 and python_version>="3.8"', ['tensorflow-aarch64']),
        # Use regular tensorflow on all other architectures or Python versions
        ('*', ['tensorflow>=2.6.0']),
    ],
    classifiers=[
        'Development Status :: 1 - Planning',
        'Intended Audience :: Qualcomm AI Developer Ecosystem',
        'License :: OSI Approved :: BSD License',  
        'Operating System :: OS Independent"',        
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.6',
        'Programming Language :: Python :: 3.8',
    ],
)
