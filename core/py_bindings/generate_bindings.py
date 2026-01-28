#!/usr/bin/env python3
"""
C++ to pybind11 Binding Generator

Parses C++ header files to find struct/class definitions and generates
pybind11 binding code.

Usage:
    python generate_bindings.py <cpp_folder> <struct_or_class_name> [--namespace <ns>] [--output <folder>]
    
Examples:
    python generate_bindings.py ../src CombatLine --namespace swtor
    python generate_bindings.py ../src Entity --namespace swtor --output ../bindings
    python generate_bindings.py ../src stat_value --namespace swtor
"""

import os
import re
import argparse
from dataclasses import dataclass, field
from typing import Optional
from pathlib import Path


# ============================================================================
# Data Classes
# ============================================================================

@dataclass
class CppField:
    """Represents a C++ class/struct field"""
    name: str
    cpp_type: str
    default_value: Optional[str] = None
    is_const: bool = False
    is_static: bool = False
    is_pointer: bool = False
    is_reference: bool = False
    is_shared_ptr: bool = False
    inner_type: Optional[str] = None  # For templates like shared_ptr<Entity>
    comment: str = ""
    access: str = "public"  # public, private, protected


@dataclass
class CppMethod:
    """Represents a C++ class/struct method"""
    name: str
    return_type: str
    params: list  # List of (type, name, default) tuples
    is_const: bool = False
    is_static: bool = False
    is_virtual: bool = False
    is_inline: bool = False
    is_override: bool = False
    comment: str = ""
    access: str = "public"


@dataclass
class CppEnum:
    """Represents a C++ enum"""
    name: str
    values: list  # List of (name, value) tuples
    is_class: bool = False  # enum class vs enum
    underlying_type: Optional[str] = None
    namespace: str = ""


@dataclass
class CppClass:
    """Represents a C++ class or struct"""
    name: str
    is_struct: bool = False
    namespace: str = ""
    fields: list = field(default_factory=list)
    methods: list = field(default_factory=list)
    base_classes: list = field(default_factory=list)
    source_file: str = ""


# ============================================================================
# C++ Parser
# ============================================================================

class CppParser:
    """Simple C++ parser for extracting struct/class definitions"""
    
    # Common C++ types that map directly to Python
    SIMPLE_TYPES = {
        'int', 'int8_t', 'int16_t', 'int32_t', 'int64_t',
        'uint8_t', 'uint16_t', 'uint32_t', 'uint64_t',
        'float', 'double', 'bool', 'char',
        'size_t', 'ssize_t'
    }
    
    STRING_TYPES = {'std::string', 'string'}
    STRING_VIEW_TYPES = {'std::string_view', 'string_view'}
    
    def __init__(self, folder_path: str):
        self.folder_path = Path(folder_path)
        self.files_content = {}
        self._load_files()
    
    def _load_files(self):
        """Load all .h and .cpp files from the folder"""
        extensions = ['.h', '.hpp', '.cpp', '.cc']
        for ext in extensions:
            for file_path in self.folder_path.rglob(f'*{ext}'):
                try:
                    with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                        self.files_content[str(file_path)] = f.read()
                except Exception as e:
                    print(f"Warning: Could not read {file_path}: {e}")
    
    def _remove_comments(self, code: str) -> str:
        """Remove C++ comments from code"""
        # Remove single-line comments but preserve them for extraction
        code = re.sub(r'//.*$', '', code, flags=re.MULTILINE)
        # Remove multi-line comments
        code = re.sub(r'/\*.*?\*/', '', code, flags=re.DOTALL)
        return code
    
    def _extract_comment_above(self, code: str, pos: int) -> str:
        """Extract documentation comment above a position"""
        # Look backwards for /// or /** comments
        lines = code[:pos].split('\n')
        comments = []
        for line in reversed(lines[-10:]):  # Look at last 10 lines
            line = line.strip()
            if line.startswith('///'):
                comments.insert(0, line[3:].strip())
            elif line.startswith('*') and not line.startswith('*/'):
                comments.insert(0, line[1:].strip())
            elif line.startswith('/**'):
                comments.insert(0, line[3:].strip())
                break
            elif line == '' or line.startswith('//'):
                continue
            else:
                break
        return ' '.join(comments)
    
    def find_class_or_struct(self, name: str, namespace: str = "") -> Optional[CppClass]:
        """Find a class or struct definition by name"""
        for file_path, content in self.files_content.items():
            result = self._parse_class_in_content(content, name, namespace, file_path)
            if result:
                return result
        return None
    
    def _parse_class_in_content(self, content: str, name: str, namespace: str, file_path: str) -> Optional[CppClass]:
        """Parse class/struct from file content"""
        
        # Build pattern to find struct/class
        # Handles: struct Name { or class Name { or struct Name : public Base {
        if namespace:
            # Look inside namespace block
            ns_pattern = rf'namespace\s+{re.escape(namespace)}\s*\{{'
            ns_match = re.search(ns_pattern, content)
            if not ns_match:
                return None
            # Find the content within this namespace (simplified - doesn't handle nested namespaces perfectly)
            start = ns_match.end()
            # We'll search from here
            search_content = content[start:]
        else:
            search_content = content
        
        # Pattern for struct/class definition
        pattern = rf'(struct|class)\s+{re.escape(name)}\s*(?::\s*(?:public|private|protected)?\s*(\w+(?:::\w+)*))?\s*\{{'
        
        match = re.search(pattern, search_content)
        if not match:
            return None
        
        is_struct = match.group(1) == 'struct'
        base_class = match.group(2) if match.group(2) else None
        
        # Find the matching closing brace
        brace_start = match.end() - 1
        body = self._extract_braced_content(search_content, brace_start)
        
        if not body:
            return None
        
        cpp_class = CppClass(
            name=name,
            is_struct=is_struct,
            namespace=namespace,
            base_classes=[base_class] if base_class else [],
            source_file=file_path
        )
        
        # Parse fields and methods
        self._parse_members(body, cpp_class, is_struct)
        
        return cpp_class
    
    def _extract_braced_content(self, content: str, start: int) -> Optional[str]:
        """Extract content between matching braces"""
        if content[start] != '{':
            return None
        
        depth = 0
        for i in range(start, len(content)):
            if content[i] == '{':
                depth += 1
            elif content[i] == '}':
                depth -= 1
                if depth == 0:
                    return content[start+1:i]
        return None
    
    def _parse_members(self, body: str, cpp_class: CppClass, is_struct: bool):
        """Parse class/struct members (fields and methods)"""
        
        # Default access for struct is public, for class is private
        current_access = "public" if is_struct else "private"
        
        # First, remove inline method bodies (content between { })
        # This prevents parsing string literals inside methods as fields
        clean_body = self._remove_inline_bodies(body)
        
        # Split by lines for easier processing
        lines = clean_body.split('\n')
        
        i = 0
        while i < len(lines):
            line = lines[i].strip()
            
            # Check for access specifier
            if line.startswith('public:'):
                current_access = 'public'
                i += 1
                continue
            elif line.startswith('private:'):
                current_access = 'private'
                i += 1
                continue
            elif line.startswith('protected:'):
                current_access = 'protected'
                i += 1
                continue
            
            # Skip empty lines, preprocessor, comments
            if not line or line.startswith('#') or line.startswith('//') or line.startswith('/*'):
                i += 1
                continue
            
            # Skip lines that are clearly not declarations
            if line.startswith('return ') or line.startswith('if ') or line.startswith('for '):
                i += 1
                continue
            
            # Try to parse as a method first (has parentheses)
            method = self._try_parse_method(line, current_access)
            if method:
                cpp_class.methods.append(method)
                i += 1
                continue
            
            # Try to parse as a field
            field = self._try_parse_field(line, current_access)
            if field:
                cpp_class.fields.append(field)
            
            i += 1
    
    def _remove_inline_bodies(self, body: str) -> str:
        """Remove inline method/function bodies to simplify parsing"""
        result = []
        i = 0
        
        while i < len(body):
            char = body[i]
            
            if char == '{':
                # Look back to determine if this is a method body or field initializer
                lookback = ''.join(result[-100:]).rstrip()  # Look at what we've collected
                
                # It's a method body if preceded by ) or ) const/override/final
                is_method_body = bool(re.search(r'\)\s*(const\s*)?(override\s*)?(final\s*)?$', lookback))
                
                if is_method_body:
                    # Skip the entire method body
                    depth = 1
                    i += 1
                    while i < len(body) and depth > 0:
                        if body[i] == '{':
                            depth += 1
                        elif body[i] == '}':
                            depth -= 1
                        i += 1
                    # Add a semicolon to mark the end of the method declaration
                    result.append(';')
                    continue
                else:
                    # It's likely a field initializer like `int x{0};` - keep it
                    result.append(char)
            else:
                result.append(char)
            
            i += 1
        
        return ''.join(result)
    
    def _try_parse_field(self, line: str, access: str) -> Optional[CppField]:
        """Try to parse a line as a field declaration"""
        
        # Remove trailing semicolon and comments
        line = re.sub(r'//.*$', '', line).strip()
        if not line.endswith(';'):
            return None
        line = line[:-1].strip()
        
        # Skip empty or very short lines
        if len(line) < 3:
            return None
        
        # Skip if it looks like a method (has parentheses not in template)
        # But allow things like std::function<...>
        paren_count = line.count('(') - line.count(')')
        if '(' in line and paren_count == 0 and not re.search(r'std::\w+<', line):
            return None
        
        # Skip using, typedef, friend, and other non-field declarations
        skip_prefixes = ('using ', 'typedef ', 'friend ', 'return ', 'if ', 'for ', 'while ', 'switch ')
        if any(line.startswith(p) for p in skip_prefixes):
            return None
        
        # Skip if the line is just a keyword
        keywords = {'const', 'static', 'virtual', 'inline', 'explicit', 'override', 'final', 
                   'public', 'private', 'protected', 'mutable', 'volatile', 'extern'}
        if line.strip() in keywords:
            return None
        
        # Parse field
        is_static = 'static ' in line
        is_const = 'const ' in line or ' const ' in line
        
        # Remove static/const for easier parsing
        clean_line = line.replace('static ', '').replace('const ', '').strip()
        
        # Skip if empty after cleaning
        if not clean_line:
            return None
        
        # Handle default values
        default_value = None
        if '=' in clean_line or '{' in clean_line:
            # Split at = or {
            if '=' in clean_line:
                parts = clean_line.split('=', 1)
                clean_line = parts[0].strip()
                default_value = parts[1].strip().rstrip('}').strip()
            elif '{' in clean_line and '}' in clean_line:
                brace_start = clean_line.index('{')
                default_value = clean_line[brace_start:].strip()
                clean_line = clean_line[:brace_start].strip()
        
        # Now parse type and name
        # Handle templates like std::shared_ptr<Entity>
        # Handle simple types like int, std::string
        
        # Try to find the name (last word)
        parts = clean_line.split()
        if len(parts) < 2:
            return None
        
        name = parts[-1].strip('*&')
        is_pointer = '*' in parts[-1] or '*' in clean_line
        is_reference = '&' in parts[-1] and '&&' not in parts[-1]
        
        # Validate name is a valid identifier
        if not re.match(r'^[a-zA-Z_][a-zA-Z0-9_]*$', name):
            return None
        
        # Skip if name is a reserved keyword
        if name in keywords:
            return None
        
        # The type is everything except the last part
        cpp_type = ' '.join(parts[:-1])
        
        # Skip if type is empty or just keywords
        if not cpp_type or cpp_type in keywords:
            return None
        
        # Check for shared_ptr, unique_ptr, vector, etc.
        is_shared_ptr = 'shared_ptr' in cpp_type
        inner_type = None
        
        template_match = re.search(r'<(.+)>', cpp_type)
        if template_match:
            inner_type = template_match.group(1).strip()
        
        return CppField(
            name=name,
            cpp_type=cpp_type,
            default_value=default_value,
            is_const=is_const,
            is_static=is_static,
            is_pointer=is_pointer,
            is_reference=is_reference,
            is_shared_ptr=is_shared_ptr,
            inner_type=inner_type,
            access=access
        )
    
    def _try_parse_method(self, line: str, access: str) -> Optional[CppMethod]:
        """Try to parse a line as a method declaration"""
        
        # Must have parentheses
        if '(' not in line:
            return None
        
        # Skip if it's just a field with std::function or similar
        if line.strip().endswith(';') and '(' in line:
            # Check if the parens are inside a template
            before_paren = line[:line.index('(')]
            if '<' in before_paren and '>' in before_paren:
                # Likely a field like std::function<void()> name;
                return None
        
        # Pattern for method: [keywords] return_type name(params) [const] [override] [= ...] ;
        # or inline definition with { }
        
        is_virtual = 'virtual ' in line
        is_static = 'static ' in line
        is_inline = 'inline ' in line
        is_const = ') const' in line or ')const' in line
        is_override = 'override' in line
        
        # Clean up keywords
        clean = line
        for kw in ['virtual ', 'static ', 'inline ', 'override', 'final']:
            clean = clean.replace(kw, '')
        clean = clean.strip()
        
        # Remove const after )
        clean = re.sub(r'\)\s*const', ')', clean)
        
        # Find the opening paren
        paren_pos = clean.find('(')
        if paren_pos == -1:
            return None
        
        before_paren = clean[:paren_pos].strip()
        
        # Split return type and name
        parts = before_paren.split()
        if not parts:
            return None
        
        name = parts[-1].strip('*&')
        return_type = ' '.join(parts[:-1]) if len(parts) > 1 else 'void'
        
        # Validate name is a valid identifier (or destructor ~Name)
        if not re.match(r'^~?[a-zA-Z_][a-zA-Z0-9_]*$', name):
            return None
        
        # Skip constructors/destructors for now (or handle them)
        if name.startswith('~') or return_type == '' or name == return_type:
            return None
        
        # Extract parameters
        paren_end = self._find_matching_paren(clean, paren_pos)
        if paren_end == -1:
            return None
        
        params_str = clean[paren_pos+1:paren_end]
        params = self._parse_params(params_str)
        
        return CppMethod(
            name=name,
            return_type=return_type,
            params=params,
            is_const=is_const,
            is_static=is_static,
            is_virtual=is_virtual,
            is_inline=is_inline,
            is_override=is_override,
            access=access
        )
    
    def _find_matching_paren(self, s: str, start: int) -> int:
        """Find matching closing parenthesis"""
        depth = 0
        for i in range(start, len(s)):
            if s[i] == '(':
                depth += 1
            elif s[i] == ')':
                depth -= 1
                if depth == 0:
                    return i
        return -1
    
    def _parse_params(self, params_str: str) -> list:
        """Parse parameter string into list of (type, name, default) tuples"""
        if not params_str.strip():
            return []
        
        params = []
        # Split by comma, but be careful of templates
        current = ""
        depth = 0
        
        for char in params_str + ',':
            if char in '<([':
                depth += 1
                current += char
            elif char in '>)]':
                depth -= 1
                current += char
            elif char == ',' and depth == 0:
                param = current.strip()
                if param:
                    parsed = self._parse_single_param(param)
                    if parsed:
                        params.append(parsed)
                current = ""
            else:
                current += char
        
        return params
    
    def _parse_single_param(self, param: str) -> Optional[tuple]:
        """Parse a single parameter into (type, name, default)"""
        default = None
        if '=' in param:
            param, default = param.split('=', 1)
            param = param.strip()
            default = default.strip()
        
        parts = param.split()
        if not parts:
            return None
        
        name = parts[-1].strip('*&') if len(parts) > 1 else ""
        ptype = ' '.join(parts[:-1]) if len(parts) > 1 else parts[0]
        
        return (ptype, name, default)
    
    def find_enum(self, name: str, namespace: str = "") -> Optional[CppEnum]:
        """Find an enum definition by name"""
        for file_path, content in self.files_content.items():
            result = self._parse_enum_in_content(content, name, namespace)
            if result:
                return result
        return None
    
    def _parse_enum_in_content(self, content: str, name: str, namespace: str) -> Optional[CppEnum]:
        """Parse enum from file content"""
        
        if namespace:
            ns_pattern = rf'namespace\s+{re.escape(namespace)}\s*\{{'
            ns_match = re.search(ns_pattern, content)
            if not ns_match:
                return None
            search_content = content[ns_match.end():]
        else:
            search_content = content
        
        # Pattern for enum
        pattern = rf'enum\s+(class\s+)?{re.escape(name)}\s*(?::\s*(\w+))?\s*\{{'
        
        match = re.search(pattern, search_content)
        if not match:
            return None
        
        is_class = match.group(1) is not None
        underlying = match.group(2)
        
        brace_start = match.end() - 1
        body = self._extract_braced_content(search_content, brace_start)
        
        if not body:
            return None
        
        # Parse enum values
        values = []
        for line in body.split(','):
            line = line.strip()
            if not line:
                continue
            
            # Remove comments
            line = re.sub(r'//.*$', '', line).strip()
            if not line:
                continue
            
            if '=' in line:
                vname, vval = line.split('=', 1)
                values.append((vname.strip(), vval.strip()))
            else:
                values.append((line, None))
        
        return CppEnum(
            name=name,
            values=values,
            is_class=is_class,
            underlying_type=underlying,
            namespace=namespace
        )


# ============================================================================
# Binding Generator
# ============================================================================

class BindingGenerator:
    """Generates pybind11 binding code from parsed C++ types"""
    
    def __init__(self):
        self.generated_types = set()  # Track what we've generated
    
    def generate_class_binding(self, cpp_class: CppClass) -> str:
        """Generate pybind11 binding for a class/struct"""
        
        lines = []
        full_name = f"{cpp_class.namespace}::{cpp_class.name}" if cpp_class.namespace else cpp_class.name
        
        # Header comment
        lines.append(f"// ============================================================================")
        lines.append(f"// Binding for {full_name}")
        lines.append(f"// Source: {cpp_class.source_file}")
        lines.append(f"// ============================================================================")
        lines.append("")
        
        # Check if it uses shared_ptr
        uses_shared_ptr = any(
            f.is_shared_ptr or 'shared_ptr' in f.cpp_type 
            for f in cpp_class.fields
        )
        
        # Class definition
        if uses_shared_ptr or any('shared_ptr' in str(m.return_type) for m in cpp_class.methods):
            lines.append(f'py::class_<{full_name}, std::shared_ptr<{full_name}>>(m, "{cpp_class.name}")')
        else:
            lines.append(f'py::class_<{full_name}>(m, "{cpp_class.name}")')
        
        # Default constructor
        lines.append(f'    .def(py::init<>())')
        
        # Public fields
        public_fields = [f for f in cpp_class.fields if f.access == 'public' and not f.is_static]
        for fld in public_fields:
            binding = self._generate_field_binding(fld, full_name)
            if binding:
                lines.append(f'    {binding}')
        
        # Public methods (non-virtual, non-override for simplicity)
        public_methods = [
            m for m in cpp_class.methods 
            if m.access == 'public' and not m.name.startswith('operator')
        ]
        
        for method in public_methods:
            binding = self._generate_method_binding(method, full_name)
            if binding:
                lines.append(f'    {binding}')
        
        # Add __repr__ method
        lines.append(self._generate_repr(cpp_class, full_name))
        
        # Close with semicolon
        lines[-1] = lines[-1].rstrip().rstrip(';') + ';'
        
        lines.append("")
        return '\n'.join(lines)
    
    def _generate_field_binding(self, fld: CppField, class_name: str) -> str:
        """Generate binding for a single field"""
        
        # Skip complex fields that need special handling
        if fld.is_static:
            return f'.def_readonly_static("{fld.name}", &{class_name}::{fld.name})'
        
        # string_view needs a lambda to convert to string
        if 'string_view' in fld.cpp_type:
            return (f'.def_property_readonly("{fld.name}", []({class_name} const& self) {{ '
                   f'return std::string(self.{fld.name}); }})')
        
        # Use def_readonly for most fields (read-only from Python)
        return f'.def_readonly("{fld.name}", &{class_name}::{fld.name})'
    
    def _generate_method_binding(self, method: CppMethod, class_name: str) -> str:
        """Generate binding for a single method"""
        
        # Skip certain methods
        if method.name in ('operator=', 'operator==', 'operator!=', 'operator<'):
            return ""
        
        # Build parameter string with defaults
        params_with_defaults = []
        for ptype, pname, default in method.params:
            if default:
                params_with_defaults.append(f'py::arg("{pname}") = {default}')
            elif pname:
                params_with_defaults.append(f'py::arg("{pname}")')
        
        params_str = ', '.join(params_with_defaults)
        
        # Handle const methods
        if method.is_const:
            method_ptr = f'static_cast<{method.return_type} ({class_name}::*)({self._params_to_types(method.params)}) const>(&{class_name}::{method.name})'
        else:
            method_ptr = f'&{class_name}::{method.name}'
        
        if params_str:
            return f'.def("{method.name}", {method_ptr}, {params_str})'
        else:
            return f'.def("{method.name}", {method_ptr})'
    
    def _params_to_types(self, params: list) -> str:
        """Convert params list to just types for function pointer cast"""
        types = [ptype for ptype, _, _ in params]
        return ', '.join(types)
    
    def _generate_repr(self, cpp_class: CppClass, full_name: str) -> str:
        """Generate __repr__ method"""
        
        # Try to find a good field to display
        display_field = None
        for candidate in ['name', 'display', 'id']:
            for f in cpp_class.fields:
                if f.name == candidate and f.access == 'public':
                    display_field = f
                    break
            if display_field:
                break
        
        if display_field and display_field.cpp_type in ('std::string', 'string'):
            return (f'    .def("__repr__", []({full_name} const& self) {{\n'
                   f'        return "<{cpp_class.name} \'" + self.{display_field.name} + "\'>";\n'
                   f'    }})')
        elif display_field:
            return (f'    .def("__repr__", []({full_name} const& self) {{\n'
                   f'        return "<{cpp_class.name} " + std::to_string(self.{display_field.name}) + ">";\n'
                   f'    }})')
        else:
            return f'    .def("__repr__", []({full_name} const&) {{ return "<{cpp_class.name}>"; }})'
    
    def generate_enum_binding(self, cpp_enum: CppEnum) -> str:
        """Generate pybind11 binding for an enum"""
        
        lines = []
        full_name = f"{cpp_enum.namespace}::{cpp_enum.name}" if cpp_enum.namespace else cpp_enum.name
        
        lines.append(f'// Enum: {full_name}')
        lines.append(f'py::enum_<{full_name}>(m, "{cpp_enum.name}")')
        
        for vname, vval in cpp_enum.values:
            # Handle Python reserved words
            py_name = vname
            if vname in ('None', 'True', 'False', 'and', 'or', 'not', 'in', 'is'):
                py_name = vname + '_'
            
            lines.append(f'    .value("{py_name}", {full_name}::{vname})')
        
        lines.append('    .export_values();')
        lines.append('')
        
        return '\n'.join(lines)
    
    def generate_header_file(self, cpp_class: CppClass, binding_code: str) -> str:
        """Generate complete header file with binding"""
        
        lines = []
        
        # Header guard
        guard_name = f"PY_BINDING_{cpp_class.name.upper()}_H"
        lines.append(f"#pragma once")
        lines.append(f"// Auto-generated pybind11 bindings for {cpp_class.name}")
        lines.append(f"// Generated by generate_bindings.py")
        lines.append("")
        lines.append("#include <pybind11/pybind11.h>")
        lines.append("#include <pybind11/stl.h>")
        lines.append("")
        
        # Include source header
        if cpp_class.source_file:
            # Make it a relative include
            source_name = os.path.basename(cpp_class.source_file)
            lines.append(f'#include "{source_name}"')
        lines.append("")
        
        lines.append("namespace py = pybind11;")
        lines.append("")
        
        # Binding function
        func_name = f"bind_{cpp_class.name}"
        lines.append(f"inline void {func_name}(py::module_& m) {{")
        lines.append("")
        
        # Indent the binding code
        for line in binding_code.split('\n'):
            if line.strip():
                lines.append("    " + line)
            else:
                lines.append("")
        
        lines.append("}")
        lines.append("")
        
        return '\n'.join(lines)


# ============================================================================
# Dependency Analyzer
# ============================================================================

class DependencyAnalyzer:
    """Analyzes dependencies between C++ types"""
    
    # Known types that don't need custom bindings
    BUILTIN_TYPES = {
        'int', 'int8_t', 'int16_t', 'int32_t', 'int64_t',
        'uint8_t', 'uint16_t', 'uint32_t', 'uint64_t',
        'float', 'double', 'bool', 'char', 'void',
        'size_t', 'ssize_t',
        'std::string', 'string',
        'std::string_view', 'string_view',
    }
    
    # STL containers to ignore (but extract their inner types)
    STL_CONTAINERS = {
        'std::vector', 'vector',
        'std::list', 'list',
        'std::map', 'map',
        'std::unordered_map', 'unordered_map',
        'std::set', 'set',
        'std::unordered_set', 'unordered_set',
        'std::shared_ptr', 'shared_ptr',
        'std::unique_ptr', 'unique_ptr',
        'std::weak_ptr', 'weak_ptr',
        'std::optional', 'optional',
        'std::pair', 'pair',
        'std::tuple', 'tuple',
        'std::function', 'function',
        'std::array', 'array',
        'std::mutex', 'mutex',
        'std::atomic', 'atomic',
        'std::thread', 'thread',
        'std::condition_variable', 'condition_variable',
        'std::lock_guard', 'lock_guard',
        'std::unique_lock', 'unique_lock',
        'std::chrono', 'chrono',
    }
    
    def __init__(self, parser: CppParser):
        self.parser = parser
    
    def find_dependencies(self, cpp_class: CppClass) -> list:
        """Find all custom types this class depends on"""
        
        deps = set()
        
        # Check field types
        for field in cpp_class.fields:
            self._extract_type_deps(field.cpp_type, deps)
            if field.inner_type:
                self._extract_type_deps(field.inner_type, deps)
        
        # Check method return types and parameters
        for method in cpp_class.methods:
            self._extract_type_deps(method.return_type, deps)
            for ptype, _, _ in method.params:
                self._extract_type_deps(ptype, deps)
        
        # Check base classes
        for base in cpp_class.base_classes:
            if base and base not in self.BUILTIN_TYPES:
                deps.add(base)
        
        # Remove the class itself
        deps.discard(cpp_class.name)
        
        return sorted(deps)
    
    def _extract_type_deps(self, type_str: str, deps: set):
        """Extract dependent type names from a type string"""
        
        if not type_str:
            return
        
        # Remove const, &, *, etc.
        type_str = type_str.replace('const ', '').replace('&', '').replace('*', '').strip()
        
        # Extract all types from nested templates
        # e.g., "std::vector<std::shared_ptr<stat_value>>" should find "stat_value"
        self._extract_all_types(type_str, deps)
    
    def _extract_all_types(self, type_str: str, deps: set):
        """Recursively extract all type names from a possibly nested template type"""
        
        if not type_str:
            return
        
        type_str = type_str.strip()
        
        # Check if this is a template type
        template_start = type_str.find('<')
        if template_start != -1:
            # Get the outer type name
            outer_type = type_str[:template_start].strip()
            
            # Find matching >
            template_end = self._find_matching_bracket(type_str, template_start)
            if template_end != -1:
                # Extract inner types
                inner = type_str[template_start + 1:template_end]
                
                # Split by comma (careful with nested templates)
                inner_types = self._split_template_args(inner)
                
                for inner_type in inner_types:
                    self._extract_all_types(inner_type.strip(), deps)
            
            # Check if outer type is a custom type (not STL)
            self._check_and_add_type(outer_type, deps)
        else:
            # No template, just a simple type
            self._check_and_add_type(type_str, deps)
    
    def _check_and_add_type(self, type_name: str, deps: set):
        """Check if a type name is a custom type and add it to deps"""
        
        if not type_name:
            return
        
        # Remove namespace prefix for checking
        simple_name = type_name.split('::')[-1]
        
        # Skip builtin types
        if type_name in self.BUILTIN_TYPES or simple_name in self.BUILTIN_TYPES:
            return
        
        # Skip STL containers
        if type_name in self.STL_CONTAINERS or simple_name in self.STL_CONTAINERS:
            return
        
        # Skip std:: types we haven't explicitly listed
        if type_name.startswith('std::'):
            return
        
        # Skip if not a valid identifier
        if not re.match(r'^[a-zA-Z_][a-zA-Z0-9_]*$', simple_name):
            return
        
        # Likely a custom type - add it
        deps.add(simple_name)
    
    def _find_matching_bracket(self, s: str, start: int) -> int:
        """Find the matching > for a < at position start"""
        depth = 0
        for i in range(start, len(s)):
            if s[i] == '<':
                depth += 1
            elif s[i] == '>':
                depth -= 1
                if depth == 0:
                    return i
        return -1
    
    def _split_template_args(self, args: str) -> list:
        """Split template arguments by comma, respecting nested templates"""
        result = []
        current = ""
        depth = 0
        
        for char in args:
            if char == '<':
                depth += 1
                current += char
            elif char == '>':
                depth -= 1
                current += char
            elif char == ',' and depth == 0:
                if current.strip():
                    result.append(current.strip())
                current = ""
            else:
                current += char
        
        if current.strip():
            result.append(current.strip())
        
        return result


# ============================================================================
# Main
# ============================================================================

def main():
    parser = argparse.ArgumentParser(
        description='Generate pybind11 bindings from C++ struct/class definitions',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
    python generate_bindings.py ../src CombatLine --namespace swtor
    python generate_bindings.py ../src Entity --namespace swtor --output ../bindings
    python generate_bindings.py ../src EventType --namespace swtor --enum
    python generate_bindings.py ../src CombatLine --namespace swtor --recursive
    python generate_bindings.py ../src CombatLine --namespace swtor --recursive --force
        """
    )
    
    parser.add_argument('folder', help='Folder containing C++ files')
    parser.add_argument('name', help='Name of struct, class, or enum to generate bindings for')
    parser.add_argument('--namespace', '-n', default='', help='C++ namespace (e.g., swtor)')
    parser.add_argument('--output', '-o', default='py_bindings', help='Output folder for generated files (default: py_bindings)')
    parser.add_argument('--enum', '-e', action='store_true', help='Generate enum binding instead of class')
    parser.add_argument('--deps', '-d', action='store_true', help='Also show dependencies')
    parser.add_argument('--stdout', '-s', action='store_true', help='Print to stdout instead of file')
    parser.add_argument('--recursive', '-r', action='store_true', help='Recursively generate bindings for all dependencies')
    parser.add_argument('--force', '-f', action='store_true', help='Overwrite existing binding files (default: preserve existing)')
    
    args = parser.parse_args()
    
    # Parse C++ files
    print(f"Scanning C++ files in: {args.folder}")
    cpp_parser = CppParser(args.folder)
    print(f"Found {len(cpp_parser.files_content)} files")
    
    generator = BindingGenerator()
    
    if args.enum:
        # Generate enum binding
        cpp_enum = cpp_parser.find_enum(args.name, args.namespace)
        if not cpp_enum:
            print(f"Error: Enum '{args.name}' not found in namespace '{args.namespace}'")
            return 1
        
        binding_code = generator.generate_enum_binding(cpp_enum)
        
        if args.stdout:
            print(binding_code)
        else:
            output_file = os.path.join(args.output, f"py_binding_{args.name.lower()}.h")
            os.makedirs(args.output, exist_ok=True)
            
            # Check if file exists and we're not forcing
            if os.path.exists(output_file) and not args.force:
                print(f"Preserved (exists): {output_file}")
            else:
                # For enums, wrap in a simple header
                full_code = f"""#pragma once
// Auto-generated pybind11 bindings for enum {args.name}

#include <pybind11/pybind11.h>

namespace py = pybind11;

inline void bind_{args.name}(py::module_& m) {{

{binding_code}
}}
"""
                with open(output_file, 'w') as f:
                    f.write(full_code)
                print(f"Generated: {output_file}")
    
    elif args.recursive:
        # Recursive generation of all dependencies
        generate_recursive(cpp_parser, generator, args.name, args.namespace, args.output, args.force)
    
    else:
        # Generate class/struct binding
        cpp_class = cpp_parser.find_class_or_struct(args.name, args.namespace)
        if not cpp_class:
            print(f"Error: Class/struct '{args.name}' not found in namespace '{args.namespace}'")
            return 1
        
        print(f"Found {cpp_class.name}: {len(cpp_class.fields)} fields, {len(cpp_class.methods)} methods")
        
        # Show dependencies if requested
        if args.deps:
            analyzer = DependencyAnalyzer(cpp_parser)
            deps = analyzer.find_dependencies(cpp_class)
            if deps:
                print(f"Dependencies: {', '.join(deps)}")
                print("Note: You should generate bindings for these types first (in order)")
        
        binding_code = generator.generate_class_binding(cpp_class)
        full_code = generator.generate_header_file(cpp_class, binding_code)
        
        if args.stdout:
            print(full_code)
        else:
            output_file = os.path.join(args.output, f"py_binding_{args.name.lower()}.h")
            os.makedirs(args.output, exist_ok=True)
            
            # Check if file exists and we're not forcing
            if os.path.exists(output_file) and not args.force:
                print(f"Preserved (exists): {output_file}")
            else:
                with open(output_file, 'w') as f:
                    f.write(full_code)
                print(f"Generated: {output_file}")
    
    return 0


def generate_recursive(cpp_parser: CppParser, generator: BindingGenerator, 
                       root_name: str, namespace: str, output_dir: str, force: bool = False):
    """Recursively generate bindings for a type and all its dependencies"""
    
    analyzer = DependencyAnalyzer(cpp_parser)
    
    # Track what we've processed and what we need to process
    processed = set()
    to_process = [root_name]
    generation_order = []  # Types in the order they should be bound
    
    print(f"\n=== Analyzing dependencies for {root_name} ===")
    
    # Build the full dependency graph
    while to_process:
        current = to_process.pop(0)
        
        if current in processed:
            continue
        
        # Try to find as class/struct first
        cpp_class = cpp_parser.find_class_or_struct(current, namespace)
        
        if cpp_class:
            # Get dependencies
            deps = analyzer.find_dependencies(cpp_class)
            
            # Add unprocessed dependencies to the front of the queue
            for dep in deps:
                if dep not in processed and dep not in to_process:
                    to_process.insert(0, dep)  # Process dependencies first
            
            processed.add(current)
            generation_order.append(('class', current, cpp_class))
            print(f"  Found class/struct: {current} (depends on: {', '.join(deps) if deps else 'nothing'})")
        else:
            # Try as enum
            cpp_enum = cpp_parser.find_enum(current, namespace)
            if cpp_enum:
                processed.add(current)
                generation_order.append(('enum', current, cpp_enum))
                print(f"  Found enum: {current}")
            else:
                print(f"  Warning: Could not find '{current}' - may be a built-in type or external")
                processed.add(current)  # Mark as processed so we don't keep trying
    
    # Reverse to get correct order (dependencies first)
    generation_order.reverse()
    
    # Remove duplicates while preserving order
    seen = set()
    unique_order = []
    for item in generation_order:
        if item[1] not in seen:
            seen.add(item[1])
            unique_order.append(item)
    
    print(f"\n=== Generation order ({len(unique_order)} types) ===")
    for kind, name, _ in unique_order:
        print(f"  {name} ({kind})")
    
    # Generate all bindings
    print(f"\n=== Generating bindings ===")
    os.makedirs(output_dir, exist_ok=True)
    
    generated_files = []
    bind_calls = []
    includes = set()
    preserved_count = 0
    generated_count = 0
    
    for kind, name, obj in unique_order:
        output_file = os.path.join(output_dir, f"py_binding_{name.lower()}.h")
        
        # Check if file already exists
        if os.path.exists(output_file) and not force:
            print(f"  Preserved (exists): {output_file}")
            preserved_count += 1
            # Still add to the includes and bind calls
            generated_files.append(f"py_binding_{name.lower()}.h")
            bind_calls.append(f"bind_{name}(m);")
            # Try to extract source file from existing binding
            if kind == 'class' and obj.source_file:
                includes.add(os.path.basename(obj.source_file))
            continue
        
        if kind == 'enum':
            binding_code = generator.generate_enum_binding(obj)
            full_code = f"""#pragma once
// Auto-generated pybind11 bindings for enum {name}

#include <pybind11/pybind11.h>

namespace py = pybind11;

inline void bind_{name}(py::module_& m) {{

{binding_code}
}}
"""
        else:  # class/struct
            binding_code = generator.generate_class_binding(obj)
            full_code = generator.generate_header_file(obj, binding_code)
            if obj.source_file:
                includes.add(os.path.basename(obj.source_file))
        
        with open(output_file, 'w') as f:
            f.write(full_code)
        
        generated_files.append(f"py_binding_{name.lower()}.h")
        bind_calls.append(f"bind_{name}(m);")
        generated_count += 1
        print(f"  Generated: {output_file}")
    
    # Generate a master include file (always regenerate this one)
    master_file = os.path.join(output_dir, "py_bindings_all.h")
    with open(master_file, 'w') as f:
        f.write("#pragma once\n")
        f.write("// Auto-generated master include for all pybind11 bindings\n")
        f.write("// Generated by generate_bindings.py --recursive\n")
        f.write("//\n")
        f.write("// Note: Individual binding files are preserved if they exist.\n")
        f.write("// Use --force to regenerate all files.\n\n")
        
        f.write("#include <pybind11/pybind11.h>\n")
        f.write("#include <pybind11/stl.h>\n\n")
        
        # Include source headers
        f.write("// Source headers\n")
        for inc in sorted(includes):
            f.write(f'#include "{inc}"\n')
        f.write("\n")
        
        # Include generated binding headers
        f.write("// Generated binding headers\n")
        for gen_file in generated_files:
            f.write(f'#include "{gen_file}"\n')
        f.write("\n")
        
        f.write("namespace py = pybind11;\n\n")
        
        # Generate the master bind function
        f.write("/// Call this function to register all bindings\n")
        f.write("inline void bind_all(py::module_& m) {\n")
        for call in bind_calls:
            f.write(f"    {call}\n")
        f.write("}\n")
    
    print(f"\n  Generated master include: {master_file}")
    print(f"\n=== Summary ===")
    print(f"  Generated: {generated_count} files")
    print(f"  Preserved: {preserved_count} files (already existed)")
    print(f"  Total:     {len(generated_files)} binding files")
    print(f"\nUsage in your code:")
    print(f'  #include "{os.path.basename(master_file)}"')
    print(f"  ")
    print(f"  PYBIND11_EMBEDDED_MODULE(swtor, m) {{")
    print(f"      bind_all(m);")
    print(f"  }}")
    if preserved_count > 0:
        print(f"\nNote: {preserved_count} existing files were preserved.")
        print(f"      Use --force to regenerate all files.")


if __name__ == '__main__':
    exit(main())
