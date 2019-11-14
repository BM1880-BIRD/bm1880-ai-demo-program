#!/usr/bin/env python3

import pipeparser
import pipegenerator
import argparse
import sys
import cppdiscovery
import os
import hashlib
import json

class Plc:
    def __init__(self, header_paths):
        self.discover_headers(header_paths)
        self.generator = pipegenerator.Generator(self.header_map)

    def cache_path(self, path):
        md5 = hashlib.md5()
        md5.update(path.encode('utf-8'))
        return os.path.relpath(os.path.dirname(__file__)) + os.path.sep + md5.hexdigest() + '.json'

    def load_header_cache(self, path):
        cache_file = self.cache_path(path)
        if os.path.exists(cache_file):
            with open(cache_file, 'r') as f:
                return json.loads(f.read())

    def save_header_cache(self, path, data):
        cache_file = self.cache_path(path)
        with open(cache_file, 'w') as f:
            json.dump(data, f)

    def discover_headers(self, paths):
        self.header_map = {}
        discovery = cppdiscovery.Discover()
        def load_header(fullpath, filename, mapping):
            _, ext = os.path.splitext(filename)
            if ext in {'.h', '.hpp'}:
                for sym in discovery.get_symbols(fullpath):
                    mapping[sym] = filename
        for path in paths:
            cached = self.load_header_cache(path)
            if cached is not None:
                self.header_map.update(cached)
            else:
                mapping = {}
                if os.path.isfile(path):
                    load_header(path, os.path.basename(path), mapping)
                else:
                    for root, _, files in os.walk(path):
                        for f in files:
                            load_header(root + os.path.sep + f, f, mapping)
                self.save_header_cache(path, mapping)
                self.header_map.update(mapping)

    def compile(self, f):
        content = ''.join(f.readlines())
        symbols, pipeline = pipeparser.parse(content)
        print(symbols, pipeline, file=sys.stderr, sep='\n')
        return self.generator.generate(symbols, pipeline)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('FILES', type=str, nargs='+', help='path to input files. read from stdin if path is -')
    parser.add_argument('--output-path', type=str)
    parser.add_argument('--header-path', type=str)
    args = parser.parse_args()
    plc = Plc([args.header_path])

    for name in args.FILES:
        if name == '-':
            output = plc.compile(sys.stdin)
        else:
            with open(name, 'r') as f:
                output = plc.compile(f)
        if args.output_path is None:
            print(output)
        else:
            with open(args.output_path + os.path.sep + os.path.splitext(os.path.basename(name))[0] + ".cpp", 'w') as f:
                print(output, file=f)
