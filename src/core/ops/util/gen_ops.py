import io
import json
import os
import sys

def _gen_wrapper(op_type, op):
  kwargs_typedef_str = "typedef struct {{ {}; }} {}_{}_kwargs_t;"
  state_typedef_str = "typedef struct {{ {}; }} {}_{}_state_t;"
  op_kwargs_macro_str = "#define __{}_{}_ARGS_UNPACK(name) {}"
  op_state_macro_str = "#define __{}_{}_STATE_UNPACK(name) {}"
  op_wrapper_decl_str = \
    {"source":
      "int hm_source_{}(void *dest, unsigned int n_bytes, {});",
      "sink":
      "int hm_sink_{}(void *src, unsigned int n_bytes, {});",
      "dsp":
      "int hm_dsp_{}(void *src, void *dest, unsigned int n_bytes, {});"}
  op_wrapper_def_str = \
    {"source":
"""
int hm_source_{1}(void *dest, unsigned int n_bytes, {0}) {{
  {1}_{3}_kwargs_t op_args;
{2};
  {1}_{3}_state_t op_state;
  {1}_{3}_init(&op_state, &op_args);
  return {1}_source_impl(dest, n_bytes, &op_args, &op_state);
  {1}_{3}_fini(&op_state, &op_args);
}}
""",
      "sink":
"""
int hm_sink_{1}(void *src, unsigned int n_bytes, {0}) {{
  {1}_{3}_kwargs_t op_args;
{2};
  {1}_{3}_state_t op_state;
  {1}_{3}_init(&op_state, &op_args);
  return {1}_sink_impl(src, n_bytes, &op_args, &op_state);
  {1}_{3}_fini(&op_state, &op_args);
}}
""",
      "dsp":
"""
int hm_dsp_{1}(void *src, void *dest, unsigned int n_bytes, {0}) {{
  {1}_{3}_kwargs_t op_args;
{2};
  {1}_{3}_state_t op_state;
  {1}_{3}_init(&op_state, &op_args);
  return {1}_dsp_impl(src, dest, n_bytes, &op_args, &op_state);
  {1}_{3}_fini(&op_state, &op_args);
}}
"""}
  op_struct_wrapper_decl_str = \
    {"source": "hm_source_op* hm_source_{}_op(hm_format_type output_type, {});",
     "sink": "hm_sink_op* hm_sink_{}_op(hm_format_type input_type, {});",
     "dsp": "hm_dsp_op* hm_dsp_{}_op(hm_format_type input_type, hm_format_type output_type, {});"}
  op_struct_wrapper_def_str = \
    {"source":
"""
hm_source_op* hm_source_{0}_op(hm_format_type output_type, {1}) {{
  {0}_source_kwargs_t* op_args = malloc(sizeof({0}_source_kwargs_t));
  hm_source_op* new_op = (hm_source_op*)malloc(sizeof(hm_source_op));
  new_op->op_fn = {0}_source_impl;
  new_op->bytes_readable_fn = {0}_bytes_readable;
  new_op->init = {0}_source_init;
  new_op->fini = {0}_source_fini;
  new_op->output_type = output_type;
{2}
  new_op->kwargs = (void*)op_args;
{3}
  (new_op->init)(new_op->state, new_op->kwargs);
  return new_op;
}}
""",
     "sink":
"""
hm_sink_op* hm_sink_{0}_op(hm_format_type input_type, {1}) {{
  {0}_sink_kwargs_t* op_args = malloc(sizeof({0}_sink_kwargs_t));
  hm_sink_op* new_op = (hm_sink_op*)malloc(sizeof(hm_sink_op));
  new_op->op_fn = {0}_sink_impl;
  new_op->init = {0}_sink_init;
  new_op->fini = {0}_sink_fini;
  new_op->bytes_writable_fn = {0}_bytes_writable;
  new_op->input_type = input_type;
{2}
  new_op->kwargs = (void*)op_args;
{3}
  (new_op->init)(new_op->state, new_op->kwargs);
  return new_op;
}}
""",
     "dsp":
"""
hm_dsp_op* hm_dsp_{0}_op(hm_format_type input_type, hm_format_type output_type, {1}) {{
  {0}_dsp_kwargs_t* op_args = malloc(sizeof({0}_dsp_kwargs_t));
  hm_dsp_op* new_op = (hm_dsp_op*)malloc(sizeof(hm_dsp_op));
  new_op->op_fn = {0}_dsp_impl;
  new_op->init = {0}_dsp_init;
  new_op->fini = {0}_dsp_fini;
  new_op->input_type = input_type;
  new_op->output_type = output_type;
{2}
  new_op->kwargs = (void*)op_args;
{3}
  (new_op->init)(new_op->state, new_op->kwargs);
  return new_op;
}}
""",}
  op_impl_decl_str =\
    {"source": "int {0}_{1}_impl(void* dest, unsigned int n_bytes, void* kwargs, void* state);\n"
      "int {0}_bytes_readable(hm_source_op *source_op);\n"
      "int {0}_source_init(void* state, void* kwargs);\n"
      "int {0}_source_fini(void* state, void* kwargs);",
     "sink": "int {0}_{1}_impl(void* src, unsigned int n_bytes, void* kwargs, void* state);\n"
      "int {0}_bytes_writable(hm_sink_op *sink_op);\n"
      "int {0}_sink_init(void* state, void* kwargs);\n"
      "int {0}_sink_fini(void* state, void* kwargs);",
     "dsp": "int {0}_{1}_impl(void* src, void* dest, unsigned int n_bytes, void* kwargs, void* state);\n"
      "int {0}_dsp_init(void* state, void* kwargs);\n"
      "int {0}_dsp_fini(void* state, void* kwargs);",}

  if "params" in op.keys() and op["params"]:
    op_param_list = '; '.join(
      ["{} {}".format(param_type, param_name) for param_name, param_type in op["params"].items()]
    )
    kwargs_typedef = kwargs_typedef_str.format(op_param_list, op["name"], op_type)
    kwargs_macro_def = '; '.join(
      ["{0} {1} = (({2}_{3}_kwargs_t*)kwargs)->{1}".format(op["params"][param_name],
      param_name, op["name"], op_type) for param_name in op["params"].keys()]
    )
    kwargs_macro_def += ';\n'
    op_wrapper_kwargs_fill = "\n".join(
      ["  op_args.{0} = {0};".format(param) for param in op["params"].keys()]
    )
    op_kwargs_macro_def = '; '.join(
      ["{0} {1} = (({2}_{3}_kwargs_t*)(name))->{1}".format(op["params"][param_name],
      param_name, op["name"], op_type) for param_name in op["params"].keys()]
    )
    op_kwargs_macro_def += ';\n'
  else:
    op_param_list = ''
    kwargs_typedef = 'typedef struct {{}} {}_{}_kwargs_t;'.format(op["name"], op_type)
    kwargs_macro_def = ''
    op_wrapper_kwargs_fill = ''
  
  if "state" in op.keys() and op["state"]:
    op_state_list = '; '.join(
      ["{} {}".format(param_type, param_name) for param_name, param_type in op["state"].items()]
    )
    state_typedef = state_typedef_str.format(op_state_list, op["name"], op_type)
    op_state_macro_def = '; '.join(
      ["{0} state_{1} = (({2}_{3}_state_t*)(name))->{1}".format(op["state"][param_name],
      param_name, op["name"], op_type) for param_name in op["state"].keys()]
    ) + ';\n'
    op_state_malloc = '  new_op->state = calloc(sizeof({}_{}_kwargs_t),1);'.format(op["name"], op_type)
  else:
    state_typedef = 'typedef struct {{}} {}_{}_state_t;'.format(op["name"], op_type)
    op_state_macro_def = ''
    op_state_malloc = 'new_op->state = NULL;'

  op_wrapper_decl = op_wrapper_decl_str[op_type].format(op["name"], op_param_list.replace(';', ','))
  new_impl_decl = op_impl_decl_str[op_type].format(op["name"], op_type)

  op_wrapper = {}
  op_wrapper["kwargs_typedef"] = kwargs_typedef
  op_wrapper["state_typedef"] = state_typedef
  op_wrapper["op_kwargs_macro_def"] = op_kwargs_macro_str.format(op_type.upper(), op["name"].upper(), op_kwargs_macro_def)
  op_wrapper["op_state_macro_def"] = op_state_macro_str.format(op_type.upper(), op["name"].upper(), op_state_macro_def)
  op_wrapper["op_wrapper_decl"] = op_wrapper_decl
  op_wrapper["op_wrapper_def"] = op_wrapper_def_str[op_type].format(op_param_list.replace(';', ','), op["name"], op_wrapper_kwargs_fill, op_type)
  op_wrapper["op_struct_wrapper_decl"] = op_struct_wrapper_decl_str[op_type].format(op["name"], op_param_list.replace(';', ','))
  op_wrapper["op_struct_wrapper_def"] = op_struct_wrapper_def_str[op_type].format(op["name"], op_param_list.replace(';', ','), op_wrapper_kwargs_fill.replace('.', '->'), op_state_malloc)
  op_wrapper["impl_decl"] = new_impl_decl

  if not ("params" in op.keys() and op["params"]):
    #op_wrapper["kwargs_macro_def"] = ''
    op_wrapper["op_wrapper_def"] = op_wrapper["op_wrapper_def"].replace(', ) {', ') {')
    op_wrapper["op_wrapper_decl"] = op_wrapper["op_wrapper_decl"].replace(', );', ');')
    op_wrapper["op_struct_wrapper_def"] = op_wrapper["op_struct_wrapper_def"].replace(', ) {', ') {')
    op_wrapper["op_struct_wrapper_decl"] = op_wrapper["op_struct_wrapper_decl"].replace(', );', ');')

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

const char **hm_source_list();
const char **hm_sink_list();
const char **hm_dsp_list();
int hm_source_op_run(hm_source_op *op, double *dest, unsigned int n_bytes);
int hm_sink_op_run(hm_sink_op *op, double *src, unsigned int n_bytes);
int hm_dsp_op_run(hm_dsp_op *op, double *src, double *dest, unsigned int n_bytes);
int hm_source_op_delete(hm_source_op *op);
int hm_sink_op_delete(hm_sink_op *op);
int hm_dsp_op_delete(hm_dsp_op *op);

int hm_op_source_bytes_readable(hm_source_op* source_op);
int hm_op_sink_bytes_writable(hm_sink_op* sink_op);

""")
  ops_h_file.seek(0, os.SEEK_END)

  ops_internal_h_file = io.StringIO(
"""\
// This file was generated automatically with util/gen_ops.py

#ifndef HIEMAL_OPS_INTERNAL_H
#define HIEMAL_OPS_INTERNAL_H

#include "api/backend.h"
#include "api/device.h"
#include "ops.h"

#define SOURCE_OP(name) int name ##_source_impl(void* dest, unsigned int n_bytes, void *kwargs, void *state)
#define SOURCE_BYTES_READABLE_FN(name) int name ##_bytes_readable(hm_source_op *src_op)
#define SINK_OP(name) int name ##_sink_impl(void* src, unsigned int n_bytes, void *kwargs, void *state)
#define SINK_BYTES_WRITABLE_FN(name) int name ##_bytes_writable(hm_sink_op *sink_op)
#define DSP_OP(name) int name ##_dsp_impl(void* src, void* dest, unsigned int n_bytes, void *kwargs, void *state)
#define SOURCE_OP_INIT_FN(name) int name ##_source_init(void* state, void* kwargs)
#define SINK_OP_INIT_FN(name) int name ##_sink_init(void* state, void* kwargs)
#define DSP_OP_INIT_FN(name) int name ##_dsp_init(void* state, void* kwargs)
#define SOURCE_OP_FINI_FN(name) int name ##_source_fini(void* state, void* kwargs)
#define SINK_OP_FINI_FN(name) int name ##_sink_fini(void* state, void* kwargs)
#define DSP_OP_FINI_FN(name) int name ##_dsp_fini(void* state, void* kwargs)

typedef int (source_op_fn)(void*, unsigned int, void*, void*);
typedef int (source_bytes_readable_fn)(hm_source_op*);
typedef int (sink_op_fn)(void*, unsigned int, void*, void*);
typedef int (sink_bytes_writable_fn)(hm_sink_op*);
typedef int (dsp_op_fn)(void*, void*, unsigned int, void*, void*);
typedef int (op_state_init_fn)(void*, void*);
typedef int (op_state_fini_fn)(void*, void*);

struct hm_source_op {
  source_op_fn *op_fn;
  source_bytes_readable_fn *bytes_readable_fn;
  op_state_init_fn *init;
  op_state_fini_fn *fini;
  hm_format_type output_type;
  void *kwargs;
  void *state;
};

struct hm_sink_op {
  sink_op_fn *op_fn;
  sink_bytes_writable_fn *bytes_writable_fn;
  op_state_init_fn *init;
  op_state_fini_fn *fini;
  hm_format_type input_type;
  void *kwargs;
  void *state;
};

struct hm_dsp_op {
  dsp_op_fn *op_fn;
  op_state_init_fn *init;
  op_state_fini_fn *fini;
  hm_format_type input_type;
  hm_format_type output_type;
  void *kwargs;
  void *state;
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
  state_typedef_dict = {"source": [], "sink": [], "dsp": []}
  op_kwargs_macro_def_dict = {"source": [], "sink": [], "dsp": []}
  op_state_macro_def_dict = {"source": [], "sink": [], "dsp": []}
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

      kwargs_typedef_dict[op_type].append(op_wrapper["kwargs_typedef"])
      state_typedef_dict[op_type].append(op_wrapper["state_typedef"])
      op_kwargs_macro_def_dict[op_type].append(op_wrapper["op_kwargs_macro_def"])
      op_state_macro_def_dict[op_type].append(op_wrapper["op_state_macro_def"])
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
    if state_typedef_dict[op_type]:
      ops_internal_h_file.write("\n".join(state_typedef_dict[op_type]))
      ops_internal_h_file.write("\n\n")
    if op_kwargs_macro_def_dict[op_type]:
      ops_internal_h_file.write("\n".join(op_kwargs_macro_def_dict[op_type]))
    if op_state_macro_def_dict[op_type]:
      ops_internal_h_file.write("\n".join(op_state_macro_def_dict[op_type]))
      ops_internal_h_file.write("\n")
    if impl_decl_dict[op_type]:
      ops_internal_h_file.write("\n".join(impl_decl_dict[op_type]))
      ops_internal_h_file.write("\n\n")

    if op_wrapper_def_dict[op_type]:
      ops_c_file.write("\n\n".join(op_wrapper_def_dict[op_type]))
    if op_struct_wrapper_def_dict[op_type]:
      ops_c_file.write("\n\n".join(op_struct_wrapper_def_dict[op_type]))

  ops_h_file.write("#endif\n")
  ops_internal_h_file.write("\n#endif\n")
  ops_c_file.write(op_name_list_str)
  ops_c_file.write(
"""
int hm_source_op_run(hm_source_op *op, double *dest, unsigned int n_bytes) {{
  return (op->op_fn)(dest, n_bytes, op->kwargs, op->state);
}}

int hm_sink_op_run(hm_sink_op *op, double *src, unsigned int n_bytes) {{
  return (op->op_fn)(src, n_bytes, op->kwargs, op->state);
}}

int hm_dsp_op_run(hm_dsp_op *op, double *src, double *dest, unsigned int n_bytes) {{
  return (op->op_fn)(src, dest, n_bytes, op->kwargs, op->state);
}}

int hm_source_op_delete(hm_source_op *op) {{
  (op->fini)(op->state, op->kwargs);
  if (op->state) {{
    free(op->state);
  }}
  free(op->kwargs);
  free(op);
  return 0;
}}

int hm_sink_op_delete(hm_sink_op *op) {{
  (op->fini)(op->state, op->kwargs);
  if (op->state) {{
    free(op->state);
  }}
  free(op->kwargs);
  free(op);
  return 0;
}}

int hm_dsp_op_delete(hm_dsp_op *op) {{
  (op->fini)(op->state, op->kwargs);
  if (op->state) {{
    free(op->state);
  }}
  free(op->kwargs);
  free(op);
  return 0;
}}

const char **hm_source_list() {{
  return ops_list;
}}

const char **hm_sink_list() {{
  return ops_list + {};
}}

const char **hm_dsp_list() {{
  return ops_list + {};
}}
""".format(len(ops["source"])+1, len(ops["source"]) + len(ops["sink"]) + 2))
  
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