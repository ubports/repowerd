'''apport package hook for repowerd

Copyright (c) 2017 Canonical Ltd.
Author: Alexandros Frantzis <alexandros.frantzis@canonical.com>

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License version 3, as published by the Free
Software Foundation. See http://www.gnu.org/copyleft/gpl.html for the full text
of the license.
'''

from apport.hookutils import *

def add_info(report, ui=None):
    attach_file_if_exists(report, '/var/log/repowerd.log', key='repowerd.log')
    attach_file_if_exists(report, '/var/log/repowerd.log.1', key='repowerd.log.1')
