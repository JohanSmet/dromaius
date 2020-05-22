#!/usr/bin/env python3

# fixup_svg_schematic.py - Johan Smet - BSD-3-Clause (see LICENSE)

import argparse
import xml.etree.ElementTree as ET

ns = {
        'dc':"http://purl.org/dc/elements/1.1/",
        'cc':"http://creativecommons.org/ns#",
        'rdf':"http://www.w3.org/1999/02/22-rdf-syntax-ns#",
        'svg':"http://www.w3.org/2000/svg",
        'xlink':"http://www.w3.org/1999/xlink",
        'sodipodi':"http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd",
        'inkscape':"http://www.inkscape.org/namespaces/inkscape"
}

def ns_attrib(namespace : str, attrib: str):
    return f"{{{ns[namespace]}}}{attrib}"

def parse_arguments():
    parser = argparse.ArgumentParser(description='Process SVG schematic to work with Dromaius/HtmlView')
    parser.add_argument('-i', '--input', help='Input SVG-file', required=True)
    parser.add_argument('-o', '--output', help='Output SVG-file', required=True)

    return parser.parse_args()

def replace_styles(root: ET.Element, css_class: str):
    for child in root:
        if 'style' in child.attrib:
            child.attrib.pop('style')
            child.attrib['class'] = css_class


def main():
    cmd_args = parse_arguments()

    ET.register_namespace('', ns['svg'])
    for key, value in ns.items():
        ET.register_namespace(key, value)

    # read svg
    svg_tree = ET.parse(cmd_args.input)
    svg_root = svg_tree.getroot()

    # prepare default styles
    styles = {
        'text.chip': 'font-style:normal;font-weight:normal;font-size:10.5833px;line-height:1.25;font-family:sans-serif;'+
                     'fill:#000000;fill-opacity:1;stroke:none;stroke-width:0.264583',
        '.chip': 'fill:none;fill-rule:evenodd;stroke:#000000;stroke-width:0.3;stroke-miterlimit:4;stroke-dasharray:none;stroke-opacity:1;',
        '.wire': 'fill:none;stroke:#000000;stroke-width:0.264583px;stroke-linecap:butt;stroke-linejoin:miter;stroke-opacity:1;'
    }

    # modify svg
    # --> remove all 'helper' layers
    schematic = None

    for layer in svg_root.findall('svg:g', ns):
        if layer.get(ns_attrib('inkscape', 'groupmode')) == 'layer':
            if layer.get(ns_attrib('inkscape', 'label')) != 'Schematic':
                svg_root.remove(layer)
            else:
                schematic = layer

    if schematic == None:
        print("Schematic layer not found")
        exit()

    # --> fix the styles of the schematic elements
    for node in schematic.findall('svg:g', ns):
        label = node.get(ns_attrib('inkscape', 'label'))
        if label.startswith('wire#'):
            class_id = label.split('#')[1]
            styles['.' + class_id] = styles['.wire']
            replace_styles(node, class_id)
        elif label.startswith('chip#'):
            replace_styles(node, 'chip')

    # --> add a style element somewhere in the beginning
    style = ET.Element('svg:style')
    style.text = '\n'.join(['%s {%s}' % kv for kv in styles.items()])
    svg_root.insert(2, style)

    # write svg
    svg_tree.write(cmd_args.output)

if __name__ == "__main__":
    main()
