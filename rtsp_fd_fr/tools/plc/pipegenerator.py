import os
import sys

_stl_headers = {
    'vector': 'vector',
    'string': 'string'
}

_template_prefix = '''
#include <memory>
#include <signal.h>
#include "pipe_executor.hpp"
#include "data_pipe.hpp"
#include "pipeline_core.hpp"
#include "fdfr_version.h"

using namespace std;
pipeline::Executor runner;

int main(int argc, char *argv[]) {
    google::InitGoogleLogging(argv[0]);
    FDFRVersion::dump_version();

    signal(SIGINT, [] (int sig) -> void {
        runner.stop();
    });

'''

_template_suffix = '''

    runner.start();
    runner.join();
    return 0;
}'''

class Generator:
    def __init__(self, header_map):
        self._header_map = header_map
        self._header_map.update(_stl_headers)

    def generate(self, symbols, document):
        return self.headers(symbols) + '\n' + '\n'.join(document['includes']) + _template_prefix + self.build(document['pipelines']) + _template_suffix

    def headers(self, symbols):
        return '\n'.join('#include "{}"'.format(self._header_map[x]) for x in symbols if x in self._header_map)

    def build(self, pipeline):
        content = []
        def build_comp(p):
            def make_comp(comp):
                return 'make_shared<{}>({})'.format(comp[0], comp[1])
            return 'pipeline::make_sequence({})'.format(', '.join([make_comp(x) for x in p]))

        def build_pipe(next_seq, p):
            data_type = 'typename pipeline::ResolveInterface<decltype({})::element_type>::input_type'.format(next_seq)
            return 'make_shared<TupleDataPipe<{}>>({}, data_pipe_mode::{})'.format(data_type, p[0], p[1])

        for i, seq in enumerate(pipeline):
            content.append('auto s{} = {};'.format(i, build_comp(seq)))

        for i, _ in enumerate(pipeline):
            content.append('auto pl{} = pipeline::make(s{});'.format(i, i))

        content.append('runner.add_pipes({});'.format('{' + ', '.join(['pl{}'.format(i) for i in range(len(pipeline))]) + '}'))

        return '\n'.join(content)