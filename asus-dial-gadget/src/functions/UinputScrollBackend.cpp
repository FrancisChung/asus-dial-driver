#include "UinputScrollBackend.h"
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <linux/uinput.h>
#include <sys/ioctl.h>

UinputScrollBackend::UinputScrollBackend()
{
    m_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (m_fd < 0) {
        return;
    }

    ioctl(m_fd, UI_SET_EVBIT, EV_REL);
    ioctl(m_fd, UI_SET_RELBIT, REL_WHEEL);
    ioctl(m_fd, UI_SET_EVBIT, EV_SYN);

    struct uinput_setup setup{};
    setup.id.bustype = BUS_VIRTUAL;
    setup.id.vendor = 0x1234;
    setup.id.product = 0x5678;
    std::strncpy(setup.name, "asus-dial-gadget-scroll", sizeof(setup.name) - 1);

    if (ioctl(m_fd, UI_DEV_SETUP, &setup) < 0 || ioctl(m_fd, UI_DEV_CREATE) < 0) {
        close(m_fd);
        m_fd = -1;
    }
}

UinputScrollBackend::~UinputScrollBackend()
{
    if (m_fd >= 0) {
        ioctl(m_fd, UI_DEV_DESTROY);
        close(m_fd);
    }
}

bool UinputScrollBackend::isAvailable() const
{
    return m_fd >= 0;
}

void UinputScrollBackend::scroll(int direction)
{
    if (m_fd < 0) {
        return;
    }

    struct input_event event{};
    event.type = EV_REL;
    event.code = REL_WHEEL;
    event.value = direction;
    write(m_fd, &event, sizeof(event));

    struct input_event sync{};
    sync.type = EV_SYN;
    sync.code = SYN_REPORT;
    sync.value = 0;
    write(m_fd, &sync, sizeof(sync));
}
