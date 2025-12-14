import io
import json
import os
import sys

def _gen_wrapper(op_type, op):
  kwargs_typedef_str = "typedef struct {{ {}; }} {}_kwargs_t;"
  kwargs_macro_str = "#define __{}_ARGS_UNPACK {}"
  op_wrapper_decl_str = \
    {"source":
      "int hm_{}(void *dest, unsigned int n_bytes, {});",
      "sink":
      "int hm_{}(void *src, unsigned int n_bytes, {});",
      "dsp":
      "int hm_{}(void *src, void *dest, unsigned int n_bytes, {});"}
  op_wrapper_def_str = \
    {"source":
"""
int hm_{1}(void *dest, unsigned int n_bytes, {0}) {{
  {1}_kwargs_t op_args;
{2};
  {1}_impl_impl(dest, n_bytes, &op_args);
}}
""",
      "sink":
"""
int hm_{1}(void *src, unsigned int n_bytes, {0}) {{
  {1}_kwargs_t op_args;
{2};
  {1}_impl(src, n_bytes, &op_args);
}}
""",
      "dsp":
"""
int hm_{1}(void *src, void *dest, unsigned int n_bytes, {0}) {{
  {1}_kwargs_t op_args;
{2};
  {1}_impl(src, dest, n_bytes, &op_args);
}}
"""}
  op_struct_wrapper_decl_str = \
    {"source": "hm_source_op* hm_{}_op(hm_format_type output_type, {});",
     "sink": "hm_source_op* hm_{}_op(hm_format_type input_type, {});",
     "dsp": "hm_dsp_op* hm_{}_op(hm_format_type input_type, hm_format_type output_type, {});"}
  op_struct_wrapper_def_str = \
    {"source":
"""
hm_source_op* hm_{0}_op(hm_format_type output_type, {1}) {{
  {0}_kwargs_t* op_args = malloc(sizeof({0}_kwargs_t));
  hm_source_op* new_op = (hm_source_op*)malloc(sizeof(hm_source_op));
  new_op->op_fn = {0}_impl;
  new_op->output_type = output_type;
{2}
  new_op->kwargs = (void*)op_args;
  return new_op;
}}
""",
     "sink":
"""
hm_sink_op* hm_{0}_op(hm_format_type input_type, {1}) {{
  {0}_kwargs_t* op_args = malloc(sizeof({0}_kwargs_t));
  hm_sink_op* new_op = (hm_sink_op*)malloc(sizeof(hm_sink_op));
  new_op->op_fn = {0}_impl;
  new_op->input_type = input_type;
{2}
  new_op->kwargs = (void*)op_args;
  return new_op;
}}
""",
     "dsp":
"""
hm_dsp_op* hm_{0}_op(hm_format_type input_type, hm_format_type output_type, {1}) {{
  {0}_kwargs_t* op_args = malloc(sizeof({0}_kwargs_t));
  hm_dsp_op* new_op = (hm_dsp_op*)malloc(sizeof(hm_dsp_op));
  new_op->op_fn = {0}_impl;
  new_op->input_type = input_type;
  new_op->output_type = output_type;
{2}
  new_op->kwargs = (void*)op_args;
  return new_op;
}}
""",}
  op_impl_decl_str =\
    {"source": "int {}_impl(void* dest, unsigned int n_bytes, void* kwargs);",
     "sink": "int {}_impl(void* src, unsigned int n_bytes, void* kwargs);",
     "dsp": "int {}_impl(void* src, void* dest, unsigned int n_bytes, void* kwargs);"}

  op_param_list = '; '.join(
    ["{} {}".format(param_type, param_name) for param_name, param_type in op["params"].items()]
  )
  kwargs_typedef = kwargs_typedef_str.format(op_param_list, op["name"])
  kwargs_macro_def = '; '.join(
    ["{0} {1} = (({2}_kwargs_t*)kwargs)->{1}".format(op["params"][param_name], 
      param_name, op["name"]) for param_name in op["params"].keys()]
  )
  kwargs_macro_def += ';\n'
  op_wrapper_decl = op_wrapper_decl_str[op_type].format(op["name"], op_param_list.replace(';', ','))
  op_wrapper_kwargs_fill = "\n".join(
    ["  op_args.{0} = {0};".format(param) for param in op["params"].keys()]
  )
  new_impl_decl = op_impl_decl_str[op_type].format(op["name"])

  op_wrapper = {}
  op_wrapper["kwargs_macro_def"] = kwargs_macro_str.format(op["name"].upper(), kwargs_macro_def)
  op_wrapper["kwargs_typedef"] = kwargs_typedef
  op_wrapper["op_wrapper_decl"] = op_wrapper_decl
  op_wrapper["op_wrapper_def"] = op_wrapper_def_str[op_type].format(op_param_list.replace(';', ','), op["name"], op_wrapper_kwargs_fill)
  op_wrapper["op_struct_wrapper_decl"] = op_struct_wrapper_decl_str[op_type].format(op["name"], op_param_list.replace(';', ','))
  op_wrapper["op_struct_wrapper_def"] = op_struct_wrapper_def_str[op_type].format(op["name"], op_param_list.replace(';', ','), op_wrapper_kwargs_fill.replace('.', '->'))
  op_wrapper["impl_decl"] = new_impl_decl
  return op_wrapper

def gen_op_wrappers(ops_file, dry_run, out_dir):
  if isinstance(out_dir, list):
    out_dir = out_dir[0]
  if (out_dir != None) and (out_dir[-1] != '/'):
    out_dir += '/'
  with open(ops_file, 'r') as f:
    ops = json.load(f)

  ops_h_file = io.StringIO(
"""\
// This file was generated automatically with util/gen_ops.py

#ifndef HIEMAL_OPS_H
#define HIEMAL_OPS_H

typedef enum {
  S16LE_I=1,
  S16LE_N,
  F16LE_I,
  F16LE_N,
  DEFAULT_FORMAT
} hm_format_type;

typedef struct hm_source_op hm_source_op;
typedef struct hm_sink_op hm_sink_op;
typedef struct hm_dsp_op hm_dsp_op;

const char **hm_op_list();
int hm_op_run(hm_dsp_op *op, double *src, double *dest, unsigned int n_bytes);
int hm_op_delete(hm_dsp_op *op);

""")
  ops_h_file.seek(0, os.SEEK_END)

  ops_internal_h_file = io.StringIO(
"""\
// This file was generated automatically with util/gen_ops.py

#ifndef HIEMAL_OPS_INTERNAL_H
#define HIEMAL_OPS_INTERNAL_H

#include "ops.h"

#define SOURCE_OP(name) int name ##_impl(void* dest, unsigned int n_bytes, void *kwargs)
#define SINK_OP(name) int name ##_impl(void* src, unsigned int n_bytes, void *kwargs)
#define DSP_OP(name) int name ##_impl(void* src, void* dest, unsigned int n_bytes, void *kwargs)

typedef int (source_op_fn)(void*, unsigned int, void*);
typedef int (sink_op_fn)(void*, unsigned int, void*);
typedef int (dsp_op_fn)(void*, void*, unsigned int, void*);

struct hm_source_op {
  source_op_fn *op_fn;
  hm_format_type output_type;
  void *kwargs;
};

struct hm_sink_op {
  sink_op_fn *op_fn;
  hm_format_type input_type;
  void *kwargs;
};

struct hm_dsp_op {
  dsp_op_fn *op_fn;
  hm_format_type input_type;
  hm_format_type output_type;
  void *kwargs;
};

""")
  ops_internal_h_file.seek(0, os.SEEK_END)

  ops_c_file = io.StringIO(
"""\
// This file was generated automatically with util/gen_ops.py

#include <stdlib.h>

#include "ops_internal.h"

""")
  ops_c_file.seek(0, os.SEEK_END)

  kwargs_typedef_dict = {"source": [], "sink": [], "dsp": []}
  kwargs_macro_def_dict = {"source": [], "sink": [], "dsp": []}
  op_wrapper_decl_dict = {"source": [], "sink": [], "dsp": []}
  op_wrapper_def_dict = {"source": [], "sink": [], "dsp": []}
  op_struct_wrapper_decl_dict = {"source": [], "sink": [], "dsp": []}
  op_struct_wrapper_def_dict = {"source": [], "sink": [], "dsp": []}
  impl_decl_dict = {"source": [], "sink": [], "dsp": []}

  op_name_list_str = "\nstatic const char* ops_list[{}] = {{\n".format(len(ops["source"]) + len(ops["sink"]) + len(ops["dsp"]) + 3)
  for op_type, op_list in ops.items():
    for op in op_list:
      op_name_list_str += '  "{}",\n'.format(op["name"])
      op_wrapper = _gen_wrapper(op_type, op)

      kwargs_macro_def_dict[op_type].append(op_wrapper["kwargs_macro_def"])
      kwargs_typedef_dict[op_type].append(op_wrapper["kwargs_typedef"])
      op_wrapper_decl_dict[op_type].append(op_wrapper["op_wrapper_decl"])
      op_wrapper_def_dict[op_type].append(op_wrapper["op_wrapper_def"])
      op_struct_wrapper_decl_dict[op_type].append(op_wrapper["op_struct_wrapper_decl"])
      op_struct_wrapper_def_dict[op_type].append(op_wrapper["op_struct_wrapper_def"])
      impl_decl_dict[op_type].append(op_wrapper["impl_decl"])
    op_name_list_str += "  NULL,\n"
  op_name_list_str += "};"

  for op_type in ["source", "sink", "dsp"]:
    ops_h_file.write("// {} \n\n".format(op_type))
    if op_wrapper_decl_dict[op_type]:
      ops_h_file.write("\n".join(op_wrapper_decl_dict[op_type]))
      ops_h_file.write("\n\n")
    if op_struct_wrapper_decl_dict[op_type]:
      ops_h_file.write("\n".join(op_struct_wrapper_decl_dict[op_type]))
      ops_h_file.write("\n\n")

    if kwargs_typedef_dict[op_type]:
      ops_internal_h_file.write("\n".join(kwargs_typedef_dict[op_type]))
      ops_internal_h_file.write("\n\n")
    if kwargs_macro_def_dict[op_type]:
      ops_internal_h_file.write("\n".join(kwargs_macro_def_dict[op_type]))
      ops_internal_h_file.write("\n")
    if impl_decl_dict[op_type]:
      ops_internal_h_file.write("\n".join(impl_decl_dict[op_type]))
      ops_internal_h_file.write("\n")

    if op_wrapper_def_dict[op_type]:
      ops_c_file.write("\n\n".join(op_wrapper_def_dict[op_type]))
    if op_struct_wrapper_def_dict[op_type]:
      ops_c_file.write("\n\n".join(op_struct_wrapper_def_dict[op_type]))

  ops_h_file.write("#endif\n")
  ops_internal_h_file.write("\n#endif\n")
  ops_c_file.write(op_name_list_str)
  ops_c_file.write(
"""
int hm_op_run(hm_dsp_op *op, double *src, double *dest, unsigned int n_bytes) {
  return (op->op_fn)(src, dest, n_bytes, op->kwargs);
}

int hm_op_delete(hm_dsp_op *op) {
  free(op->kwargs);
  free(op);
  return 0;
}

const char **hm_op_list() {
  return ops_list;
}
""")
  
  ops_h_file.seek(0)
  ops_internal_h_file.seek(0)
  ops_c_file.seek(0)

  if dry_run:
    sys.stdout.write("--- ops.h ---\n")
    sys.stdout.write(ops_h_file.read())
    sys.stdout.write("\n--- ops_internal.h ---\n")
    sys.stdout.write(ops_internal_h_file.read())
    sys.stdout.write("\n--- ops.c ---\n")
    sys.stdout.write(ops_c_file.read())
  elif out_dir is None:
    raise ValueError("no out_dir specified")
  else:
    with open(out_dir + "ops.h", 'w') as f:
      f.write(ops_h_file.read())
    with open(out_dir + "ops_internal.h", 'w') as f:
      f.write(ops_internal_h_file.read())
    with open(out_dir + "ops.c", 'w') as f:
      f.write(ops_c_file.read())          

if __name__ == "__main__":
  import argparse
  parser = argparse.ArgumentParser()
  parser.add_argument('-l', '--list', action='store_true')
  parser.add_argument('-d', '--dry-run', action='store_true')
  parser.add_argument('-i', '--ops-file', default='ops.json', action='store', nargs=1)
  parser.add_argument('-o', '--out-dir', action='store', nargs=1)
  args = parser.parse_args(sys.argv[1:])
  gen_op_wrappers(args.ops_file, args.dry_run, args.out_dir)