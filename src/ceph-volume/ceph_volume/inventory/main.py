# -*- coding: utf-8 -*-

import argparse
import json

from ceph_volume.util.device import Devices, Device


class Inventory(object):

    help = "Get this nodes available disk inventory"

    def __init__(self, argv):
        self.argv = argv

    def main(self):
        parser = argparse.ArgumentParser(
            prog='ceph-volume inventory',
            formatter_class=argparse.RawDescriptionHelpFormatter,
            description=self.help,
        )
        parser.add_argument(
            'path',
            nargs='?',
            default=None,
            help=('Report on specific disk'),
        )
        parser.add_argument(
            '--format',
            choices=['plain', 'json', 'json-pretty'],
            default='plain',
            help='Output format',
        )
        parser.add_argument(
            '--filter-for-batch',
            action='store_true',
            help=('Filter devices unsuitable to pass to an OSD service spec, '
                  'no effect when <path> is passed'),
            default=False,
        )
        self.args = parser.parse_args(self.argv)
        if self.args.path:
            self.format_report(Device(self.args.path))
        else:
            self.format_report(Devices(self.args.filter_for_batch))

    def get_report(self):
        if self.args.path:
            return Device(self.args.path).json_report()
        else:
            return Devices(self.args.filter_for_batch).json_report()

    def format_report(self, inventory):
        if self.args.format == 'json':
            print(json.dumps(inventory.json_report()))
        elif self.args.format == 'json-pretty':
            print(json.dumps(inventory.json_report(), indent=4, sort_keys=True))
        else:
            print(inventory.pretty_report())
