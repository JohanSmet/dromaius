#!/usr/bin/env python3

# fixup_svg_keyboard.py - Johan Smet - BSD-3-Clause (see LICENSE)

import argparse
import os.path
from lxml import etree as ET

styles = { }
reverse_styles = { }

style_counter = {
    'text': 0,
    'tspan': 0,
    'ellipse': 0,
    'g': 0,
    'path': 0,
    'rect': 0,
    'circle': 0
}

id_counter = {}

def parse_arguments():
    parser = argparse.ArgumentParser(description='Process SVG to work with Dromaius/HtmlView')
    parser.add_argument('-i', '--input', help='Input SVG-file', required=True)
    parser.add_argument('-o', '--output', help='Output SVG-file', required=True)

    return parser.parse_args()

def fetch_attribute_value(node: ET.Element, attr: str, namespace: str = None):
    if namespace is not None:
        full = ET.QName(node.nsmap[namespace], attr)
        attr = full.text

    return node.get(attr)

def remove_attribute(node: ET.Element, attr: str, namespace: str = None):
    if namespace is not None:
        full = ET.QName(node.nsmap[namespace], attr)
        attr = full.text

    if attr in node.attrib:
        node.attrib.pop(attr)

def unique_css_class_for_style_string(el_type: str, style: str) -> str:
    if el_type == 'tspan':
        style = 'pointer-events:none;' + style

    if style in reverse_styles:
        return reverse_styles[style]

    class_name = f'{el_type}_{style_counter[el_type]}'

    reverse_styles[style] = class_name
    styles['.' + class_name] = style
    style_counter[el_type] += 1

    return class_name

def unique_id_for_node(node: ET.Element, el_type: str) -> str:
    base = fetch_attribute_value(node, 'label', 'inkscape')
    if base is None:
        base = el_type

    if base in id_counter:
        suffix = id_counter[base]
        id_counter[base] = suffix + 1
        return f'{base}_{suffix}'
    else:
        id_counter[base] = 1;
        return base

def process_node_styles(root: ET.Element):
    for node in root:
        tag = ET.QName(node).localname

        node.attrib['id'] = unique_id_for_node(node, tag)
        remove_attribute(node, 'role', 'sodipodi')
        remove_attribute(node, 'nodetypes', 'sodipodi')
        remove_attribute(node, 'transform-center-x', 'inkscape')
        remove_attribute(node, 'transform-center-y', 'inkscape')

        process_node_styles(node);

        if 'style' in node.attrib:
            node.attrib['class'] = unique_css_class_for_style_string(tag, fetch_attribute_value(node, 'style'))
            remove_attribute(node, 'style')

        remove_attribute(node, 'label', 'inkscape')

def main():

    cmd_args = parse_arguments()

    # read svg
    svg_tree = ET.parse(cmd_args.input)
    svg_root = svg_tree.getroot()

    # modify svg
    # --> remove inkspace specific elements
    for node in svg_root.findall('{*}metadata'):
        svg_root.remove(node)
    for node in svg_root.findall('{*}namedview'):
        svg_root.remove(node)

    # --> cleanup defs
    for defs in svg_root.findall('{*}defs'):
        for def_node in defs:
            remove_attribute(def_node, 'isstock', 'inkscape')
            remove_attribute(def_node, 'stockid', 'inkscape')

    # --> cleanup root element
    remove_attribute(svg_root, 'width')
    remove_attribute(svg_root, 'height')
    remove_attribute(svg_root, 'docname', 'sodipodi')
    remove_attribute(svg_root, 'version', 'inkscape')

    # --> process all layers
    for layer in svg_root.findall('svg:g', svg_root.nsmap):
        layer.attrib['id'] = fetch_attribute_value(layer, 'label', 'inkscape')
        remove_attribute(layer, 'label', 'inkscape')
        remove_attribute(layer, 'groupmode', 'inkscape')

        # --> merge the styles of the elements
        process_node_styles(layer)

    # --> add a style element somewhere in the beginning
    style = ET.Element('style')
    style.text = '\n'.join(['%s {%s}' % kv for kv in styles.items()])
    svg_root.insert(2, style)

    # --> remove unused namespaces
    ET.cleanup_namespaces(svg_root)

    # write svg
    svg_tree.write(cmd_args.output, pretty_print=True)

if __name__ == "__main__":
    main()
