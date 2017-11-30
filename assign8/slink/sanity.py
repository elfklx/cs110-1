#!/usr/bin/python
import re
def filter_sunet(str):
    str = re.sub('.* hashes to', '<path> hashes to', str)
    return str

def strip_X11_forwarding_messages(str):
    str = filter_sunet(str)
    str = re.sub('DISPLAY.*\n', '', str)
    str = re.sub('DISPLAY.*$', '', str)
    return str

