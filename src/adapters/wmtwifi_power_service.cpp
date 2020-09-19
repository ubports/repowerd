/*
 * Copyright © 2020 UBPorts Foundation.
 * Copyright © 2018 Jolla Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "wmtwifi_power_service.h"
#include "src/core/log.h"

#include <net/if.h>
#include <netlink/errno.h>
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>

#include <linux/nl80211.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define WMTWIFI_DEVICE "/dev/wmtWifi"
#define TESTMODE_CMD_ID_SUSPEND 101
#define WMTWIFI_SUSPEND_VALUE   (1)
#define WMTWIFI_RESUME_VALUE    (0)

#define PRIV_CMD_SIZE 512
typedef struct android_wifi_priv_cmd {
    char buf[PRIV_CMD_SIZE];
    int used_len;
    int total_len;
} android_wifi_priv_cmd;

enum mce_display_events {
    DISPLAY_EVENT_VALID,
    DISPLAY_EVENT_STATE,
    DISPLAY_EVENT_COUNT
};

struct testmode_cmd_hdr {
    uint32_t idx;
    uint32_t buflen;
};

struct testmode_cmd_suspend {
    struct testmode_cmd_hdr header;
    uint8_t suspend;
};

struct multicast_group {
    const char *group;
    int id;
};

namespace
{
char const* const log_tag = "WmtWifiPowerService";
auto const null_handler = []{};

struct nl_sock *nl_socket = NULL;
int driver_id = -1;
std::shared_ptr<repowerd::Log> log;
}

static int handle_nl_command_valid(struct nl_msg*, void *arg)
{
    int *ret = (int*)arg;
    *ret = 0;
    log->log(log_tag, "handle_nl_command_valid: %d", *ret);
    return NL_SKIP;
}

static int handle_nl_command_error(struct sockaddr_nl*, struct nlmsgerr *err, void *arg)
{
    int *ret = (int*)arg;
    *ret = err->error;
    log->log(log_tag, "%s: error: %d", __func__, *ret);
    return NL_SKIP;
}

static int handle_nl_command_finished(struct nl_msg*, void *arg)
{
    int *ret = (int*)arg;
    *ret = 0;
    log->log(log_tag, "handle_nl_command_finished: %d", *ret);
    return NL_SKIP;
}

static int handle_nl_command_ack(struct nl_msg*, void *arg)
{
    int *ret = (int*)arg;
    *ret = 0;
    log->log(log_tag, "handle_nl_command_ack: %d", *ret);
    return NL_STOP;
}

static int handle_nl_seq_check(struct nl_msg*, void*)
{
    log->log(log_tag, "handle_nl_seq_check");

    return NL_OK;
}

static int suspend_plugin_netlink_handler(std::shared_ptr<repowerd::Log> const& log)
{
    struct nl_cb *cb;
    int res = 0;
    int err = 0;

    cb = nl_cb_alloc(NL_CB_VERBOSE);
    if (!cb) {
        log->log(log_tag, "%s: failed to allocate netlink callbacks", __func__);
        return 1;
    }

    err = 1;
    nl_cb_err(cb, NL_CB_CUSTOM, handle_nl_command_error, &err);
    nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, handle_nl_command_valid, &err);
    nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, handle_nl_command_finished, &err);
    nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, handle_nl_command_ack, &err);
    nl_cb_set(cb, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, handle_nl_seq_check, &err);

    while (err == 1) {
        log->log(log_tag, "waiting until nl testmode command has been processed\n");
        res = nl_recvmsgs(nl_socket, cb);
        if (res < 0) {
            log->log(log_tag, "nl_recvmsgs failed - wmtWifi %s:%d\n", __func__, res);
            break;
        }
    }

    if (err == 0) {
        log->log(log_tag, "suspend on/off successfully done");
    }

    nl_cb_put(cb);

    return err;
}

/**
 * @brief Suspend or resume wmtWifi device with gen2 or gen3 driver
 * 
 * @param ifname interface name (e.g. wlan0)
 * @param suspend_value suspend value as uint8_t (usually 1 to suspend, 0 to resume)
 */
void repowerd::suspend_set_wmtwifi(std::shared_ptr<repowerd::Log> const& log_, const char *ifname, uint8_t suspend_value)
{
    log = log_;
    // first try the vendor specific TESTMODE command
    struct nl_msg *msg = NULL;
    int ifindex = 0;
    struct testmode_cmd_suspend susp_cmd;

    int success = 0;

    ifindex = if_nametoindex(ifname);

    if (ifindex == 0) {
        log->log(log_tag, "iface %s is not active/present (handle on_off).", ifname);
        return;
    }

    log->log(log_tag, "iface: %s suspend value: %d\n", ifname, (int)suspend_value);

    msg = nlmsg_alloc();

    genlmsg_put(msg, 0, 0, driver_id, 0, 0, NL80211_CMD_TESTMODE, 0);

    memset(&susp_cmd, 0, sizeof susp_cmd);
    susp_cmd.header.idx = TESTMODE_CMD_ID_SUSPEND;
    susp_cmd.header.buflen = 0; // unused
    susp_cmd.suspend = suspend_value;

    nla_put_u32(msg, NL80211_ATTR_IFINDEX, ifindex);
    nla_put(
        msg,
        NL80211_ATTR_TESTDATA,
        sizeof susp_cmd,
        (void*)&susp_cmd);

    if (nl_send_auto(nl_socket, msg) < 0) {
        log->log(log_tag, "Failed to send testmode command.\n");
    } else {
        if (suspend_plugin_netlink_handler(log) != 0) {
            // the driver returned an error or doesn't support this command
            // could be a driver which uses "SETSUSPENDMODE 1/0" priv cmds
            log->log(log_tag, "%s: TESTMODE command failed."
                "Ignore if the kernel is using a gen3 wmtWifi driver.\n",
                __func__);
        } else {
            success = 1;
        }
    }

    nlmsg_free(msg);

    // also send SETSUSPENDMODE private commands for gen3 drivers:
    int cmd_len = 0;
    struct ifreq ifr;
    android_wifi_priv_cmd priv_cmd;

    int ret;
    int ioctl_sock;

    ioctl_sock = socket(PF_INET, SOCK_DGRAM, 0);

    memset(&ifr, 0, sizeof(ifr));
    memset(&priv_cmd, 0, sizeof(priv_cmd));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);

    cmd_len = snprintf(
        priv_cmd.buf,
        sizeof(priv_cmd.buf),
        "SETSUSPENDMODE %d",
        (int)suspend_value);

    priv_cmd.used_len = cmd_len + 1;
    priv_cmd.total_len = PRIV_CMD_SIZE;
    ifr.ifr_data = (char*)&priv_cmd;

    ret = ioctl(ioctl_sock, SIOCDEVPRIVATE + 1, &ifr);

    if (ret != 0) {
        log->log(log_tag, "%s: SETSUSPENDMODE private command failed: %d,"
            "ignore if the kernel is using a gen2 wmtWifi driver.",
            __func__,
            errno);
    } else {
        success = 1;
    }

    close(ioctl_sock);

    if (!success) {
        log->log(log_tag, "%s: could not enter suspend mode, both methods failed",
            __func__);
    }
}
