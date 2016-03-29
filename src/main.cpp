#include "core/daemon.h"
#include "default_daemon_config.h"

int main()
{
    repowerd::DefaultDaemonConfig config;
    repowerd::Daemon daemon{config};

    daemon.run();
}
