#!/usr/bin/env python3

# fixup_svg_schematic.py - Johan Smet - BSD-3-Clause (see LICENSE)

import argparse
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
color_vars = {}

def parse_arguments():
    parser = argparse.ArgumentParser(description='Process SVG schematic to work with Dromaius/HtmlView')
    parser.add_argument('-i', '--input', help='Input SVG-file', required=True)
    parser.add_argument('-o', '--output', help='Output SVG-file', required=True)

    return parser.parse_args()

def color_var_name(raw: str) -> str:
    result = raw.replace('/', 'bar')
    color_var =  '--color-' + result
    color_vars[color_var] = '#000000'
    return color_var

def replace_styles(root: ET.Element, css_class: str):
    for child in root:
        if not 'style' in child.attrib:
            continue
        if child.tag == f"{{{ns['svg']}}}text":
            continue

        styles = child.attrib['style'].split(';')
        styles_filtered = [x for x in styles if not x.startswith('stroke:')]
        child.attrib['style'] = ';'.join(styles_filtered)
        child.attrib['class'] = css_class

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

def replace_color_styles(node: ET.Element, color_var: str):
    style = fetch_attribute_value(node, 'style')

    if style is None:
        return

    styles = style.split(';')

    for idx, el in enumerate(styles):
        [key, val] = el.split(':')
        if key in ['stroke', 'fill'] and val.lower() != 'none':
            styles[idx] = f'{key}:var({color_var})'

    node.attrib['style'] = ';'.join(styles)

def unique_css_class_for_style_string(el_type: str, style: str) -> str:
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

    suffix = 0
    if base in id_counter:
        suffix = id_counter[base]
    id_counter[base] = suffix + 1

    return f'{base}_{suffix}'

def process_node_styles(root: ET.Element, group_type: str = None, color_var: str = None):
    for node in root:
        tag = ET.QName(node).localname

        node.attrib['id'] = unique_id_for_node(node, tag)

        if group_type == 'wire':
            replace_color_styles(node, color_var)

        remove_attribute(node, 'role', 'sodipodi')
        remove_attribute(node, 'nodetypes', 'sodipodi')
        remove_attribute(node, 'transform-center-x', 'inkscape')
        remove_attribute(node, 'transform-center-y', 'inkscape')

        label = fetch_attribute_value(node, 'label', 'inkscape')
        if label is not None and '#' in label:
            [n_grouptype, n_color_var] = label.split('#')
            n_color_var = color_var_name(n_color_var)

            if n_grouptype == 'wire':
                replace_color_styles(node, n_color_var)

            process_node_styles(node, n_grouptype, n_color_var)
        else:
            process_node_styles(node, group_type, color_var)

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

    # --> remove all 'helper' layers
    schematic = None

    for layer in svg_root.findall('svg:g', svg_root.nsmap):
        if fetch_attribute_value(layer, 'groupmode', 'inkscape') == 'layer':
            if fetch_attribute_value(layer, 'label', 'inkscape') != 'Schematic':
                svg_root.remove(layer)
            else:
                schematic = layer

    if schematic == None:
        print("Schematic layer not found")
        exit()

    schematic.attrib['id'] = fetch_attribute_value(schematic, 'label', 'inkscape')
    remove_attribute(schematic, 'label', 'inkscape')
    remove_attribute(schematic, 'groupmode', 'inkscape')

    # --> fix the styles of the schematic elements
    process_node_styles(schematic)

    # --> add a style element somewhere in the beginning
    styles[':root'] = ''
    for key, value in color_vars.items():
        styles[':root'] += f'{key}:{value};'

    style = ET.Element('style')
    style.text = '\n'.join(['%s {%s}' % kv for kv in styles.items()])
    svg_root.insert(2, style)

    # --> remove unused namespaces
    ET.cleanup_namespaces(svg_root)

    # write svg
    svg_tree.write(cmd_args.output, pretty_print=True)

if __name__ == "__main__":
    main()
