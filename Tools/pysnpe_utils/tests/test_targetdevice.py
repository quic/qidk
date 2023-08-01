import pytest
from ppadb.client import Client as AdbClient
from icecream import ic

from pysnpe_utils import pysnpe

@pytest.fixture
def android_device_connected():
    client = AdbClient(host="localhost", port=5037)
    devices = client.devices()
    if not devices:
        pytest.skip("No Android device connected")
    return devices[0]


def verify_pushed_assets(device, storage_loc):
    """
    Check whether SNPE assets have been pushed or not
    """
    assert device.shell(f"ls {storage_loc}/bin/snpe-net-run") == f'{storage_loc}/bin/snpe-net-run\n'
    device.shell(f"ls {storage_loc}/lib/libSNPE.so") == f'{storage_loc}/lib/libSNPE.so\n'
    device.shell(f"ls {storage_loc}/dsp/libSnpeHtpV73Skel.so") == f'{storage_loc}//dsp/libSnpeHtpV73Skel.so\n'


def test_android_targetdevice(android_device_connected):
    device = android_device_connected
    ic(device.get_serial_no())
    ic(device.get_properties()['ro.product.name'])

    assert int(device.get_properties()['ro.build.version.sdk']) >= 16

    target_device = pysnpe.TargetDevice(target_device_adb_id=device.get_serial_no(), send_root_access_request=True)
    verify_pushed_assets(device, target_device.device_storage_loc)
    device.shell(f"rm -rf {target_device.device_storage_loc}")

    target_device = pysnpe.TargetDevice()
    verify_pushed_assets(device, target_device.device_storage_loc)