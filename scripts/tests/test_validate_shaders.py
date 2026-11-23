#!/usr/bin/env python3
"""Tests for the vertex/fragment interface check in scripts/validate_shaders.py
(the rule Apple's GL enforces at link time, issue #183).

Run: python3 -m unittest discover -s scripts/tests
"""

import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
import validate_shaders as vs  # noqa: E402

FRAG = """
in PREC vec3 Normal;
flat in int Kind;
in vec2 Uv[2];
uniform sampler2D tex1;
layout (location=0) out PREC vec4 FragColor;
void main() { FragColor = vec4(Normal, 1.0); }
"""


class UnwrittenInputs(unittest.TestCase):
    def test_all_written(self):
        vert = "out vec3 Normal; flat out int Kind; out vec2 Uv[2];\n" \
               "void main() { Normal = n; Kind = 1; Uv[0] = a; Uv[1].x += b; }"
        self.assertEqual(vs.unwritten_inputs(vert, FRAG), [])

    def test_declared_but_never_assigned(self):
        vert = "out vec3 Normal; flat out int Kind; out vec2 Uv[2];\n" \
               "void main() { Kind = 1; Uv[0] = a; }"
        self.assertEqual(vs.unwritten_inputs(vert, FRAG), ['Normal'])

    def test_a_commented_assignment_does_not_count(self):
        vert = "out vec3 Normal;\n" \
               "void main() {\n    //Normal = n;\n    /* Normal = n; */\n    Kind = 1; Uv[0] = a; }"
        self.assertEqual(vs.unwritten_inputs(vert, FRAG), ['Normal'])

    def test_a_comparison_is_not_an_assignment(self):
        vert = "void main() { if (Normal == n) {} Kind = 1; Uv[0] = a; }"
        self.assertEqual(vs.unwritten_inputs(vert, FRAG), ['Normal'])

    def test_layout_qualified_input_is_checked(self):
        frag = "layout(location = 1) in vec3 Tangent;\nvoid main() {}"
        self.assertEqual(vs.unwritten_inputs("void main() {}", frag), ['Tangent'])
        self.assertEqual(vs.unwritten_inputs("void main() { Tangent = t; }", frag), [])

    def test_vertex_attributes_are_not_fragment_inputs(self):
        # a vertex shader's own "in" lines are attributes, not checked here
        frag = "layout (location=0) out vec4 FragColor;\nvoid main() {}"
        self.assertEqual(vs.unwritten_inputs("layout(location = 0) in vec4 pos;", frag), [])


if __name__ == '__main__':
    unittest.main()
