#include <monapi.h>
#define MUNIT_GLOBAL_VALUE_DEFINED
#include <monapi/MUnit.h>

#include <drivers/virtio/VirtioDevice.h>

static void test_probe()
{
    VirtioDevice* vdev = VirtioDevice::probe(PCI_DEVICE_ID_VIRTIO_NET);
    ASSERT_TRUE(vdev != NULL);
}

int main(int argc, char *argv[])
{
    test_probe();

    TEST_RESULTS(virtio);
    return 0;
}
