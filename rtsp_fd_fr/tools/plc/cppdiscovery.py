#!/usr/bin/env python3
import clang.cindex
from clang.cindex import CursorKind

clang.cindex.Config.set_library_file('/usr/lib/llvm-6.0/lib/libclang.so.1')

class Discover:
    def __init__(self):
        self.index = clang.cindex.Index.create()

    def get_symbols(self, filename):
        # tu = self.index.parse(filename)
        tu = self.index.parse(filename, args=['-ast-dump-filter={}'.format(filename)])
        return self._traverse(filename, tu.cursor)

    def _traverse(self, filename, node, namespaces = []):
        result = []
        if node.location.file is not None and node.location.file.name != filename:
            return []
        if node.is_definition():
            if node.kind in {CursorKind.CLASS_DECL, CursorKind.CLASS_TEMPLATE, CursorKind.STRUCT_DECL}:
                result.append('::'.join(namespaces + [node.spelling]))
                namespaces = namespaces + [node.spelling]
            elif node.kind == CursorKind.NAMESPACE:
                namespaces = namespaces + [node.spelling]
            elif node.kind in {CursorKind.TYPE_ALIAS_DECL, CursorKind.TYPE_ALIAS_TEMPLATE_DECL, CursorKind.ENUM_CONSTANT_DECL}:
                result.append('::'.join(namespaces + [node.spelling]))
            elif node.kind == CursorKind.ENUM_DECL:
                if not node.is_scoped_enum():
                    for child in node.get_children():
                        result.extend(self._traverse(filename, child, namespaces))
                namespaces = namespaces + [node.spelling]

        for child in node.get_children():
            result.extend(self._traverse(filename, child, namespaces))
        return result
