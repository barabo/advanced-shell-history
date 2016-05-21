#!/usr/bin/python
#
# Copyright 2012 Carl Anderson
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
"""A helper library to expose Unix system information."""

import os
import pwd
import re
import socket
import sys
import time


def GetCWD():
  """Returns the current working directory."""
  return os.getcwd()


def GetEnv(variable):
  """Returns the environment variable value as a string."""
  return os.getenv(variable)


def GetEnvInt(variable):
  """Returns the environment variable value as an integer."""
  return int(os.getenv(variable) or 0)


def GetEUID():
  """Returns the current effective user ID as an int."""
  return os.geteuid()


def _GetIfconfig():
  """Returns the lines emitted by /sbin/ifconfig -a"""
  try:
    fd = subprocess.Popen(
        "/sbin/ifconfig -a",
        shell=True,
        bufsize=8000,
        stdout=subprocess.PIPE).stdout
    return [x.lower().rstrip() for x in fd.readlines()]
  except Error:
    return None


def _ParseIfconfig():
  """Returns a dict of devices to ip addresses for this machine."""
  device_matcher = re.compile(r'([^:\s]+)[:\s]')
  address_matcher = re.compile(r'\s+(inet6?\s)(addr:)?\s?([^\s/%]+)')
  mac_matcher = re.compile(r'.*\s(hwaddr|ether)\s([0-9a-f:]+)')

  device = None
  devices = {}
  inet_addresses = []
  mac_address = ''

  for line in _GetIfconfig():
    if device_matcher.match(line):
      if device:
        devices['%s|%s' % (device, mac_address)] = inet_addresses
        inet_addresses = []
        mac_address = ''
      device = device_matcher.match(line).groups()[0]
    elif address_matcher.match(line):
      inet_addresses.append(address_matcher.match(line).groups()[2])
    if mac_matcher.match(line):
      mac_address = mac_matcher.match(line).groups()[1]

  return devices


def GetHostIp():
  """Returns the ip addresses for this host."""
  ips = []
  # TODO(cpa): respect the relevant ASH_CFG_ settings here.
  for addresses in _ParseIfconfig().itervalues():
    for address in addresses:
      ips.append(address)
  return ' '.join(ips)


def GetHostName():
  """Returns the hostname."""
  return socket.gethostname()


def GetLoginName():
  """Returns the user login name."""
  return pwd.getpwuid(os.getuid())[0]


def GetPID():
  """Returns the PID of the shell."""
  return os.getppid()


def GetPPID():
  """Returns the PPID of the shell."""
  return _GetProcStat(3)


def _GetProcStat(num):
  """Returns the i'th field of /proc/<pid>/stat of the shell."""
  stat_file = '/proc/%d/stat' % os.getppid()
  if not os.path.exists(stat_file):
    return ''
  with open(stat_file) as fd:
    data = fd.read()
    return data.split(' ', num + 1)[num]


def GetShell():
  """Returns the name of the shell (ie: either zsh or bash)"""
  return _GetProcStat(1).strip('()')


def GetTime():
  """Returns the epoch timestamp."""
  return long(time.time())


def GetTimeZone():
  """Returns the local time zone string."""
  return time.tzname[time.localtime()[8]]


def GetTTY():
  """Return the name of the current controlling tty."""
  tty_name = os.ttyname(sys.stdin.fileno())
  if tty_name and tty_name.startswith('/dev/'):
    return tty_name[5:]
  return tty_name


def GetUID():
  """Returns the UID of the command."""
  return os.getuid()

