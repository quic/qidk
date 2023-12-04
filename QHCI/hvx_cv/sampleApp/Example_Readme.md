### Example to integrate libqhci.so  into  project



This example is based in QNX platform, SA8295.

#### Prepare third-part  libs and header files

##### 1 make a folder to store third-part libs and header files

```
make third_part/
make third_part/libs
make third_part/rpcmem
```

##### 2 pull libcdsprpc.so/libfastrpc_pmem.so/liblibstd.so/libsmmu_client.so from device or from hexagon sdk and push to third_part/libs

##### 3 push release/libs/aarch64-qnx/libqhci.so to third_part/libs

##### 4 pull rpcmem.h/remote.h/remote_default.h/AEEStdDef.h from hexagon sdk or from qnx build and push to  third_part/rpcmem

##### 5 push release/API/qhci.h to  third_part/rpcmem



```
#cd third_part
#tree
|-- libs
|   |-- libcdsprpc.so
|   |-- libfastrpc_pmem.so
|   |-- liblibstd.so
|   |-- libqhci.so
|   `-- libsmmu_client.so
`-- rpcmem
    |-- AEEStdDef.h
    |-- qhci.h
    |-- remote.h
    |-- remote_default.h
    `-- rpcmem.h
```

#### build the example

```
mkdir build
cd build
cmake ..
make
```



#### testing:

1 push release/libs/aarch64-qnx/libqhci.so and release/libs/hexagon/libqhci_skel.so to device

```
mount -uw /mnt

push release/libs/aarch64-qnx/libqhci.so to /mnt/lib64/
push release/libs/hexagon-v68/libqhci_skel.so to /mnt/etc/images/dsp/
push demo to /data/local/tmp/
```



```
./demo

....
resize_near execute time: 6000 us result 0
....
```

